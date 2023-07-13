#include "transpiler/rust/yosys_transpiler.h"

#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "google/protobuf/util/internal/utility.h"
#include "transpiler/netlist_utils.h"
#include "transpiler/rust/tfhe_rs_templates.h"
#include "xls/common/status/status_macros.h"
// This comment will include status matchers externally.  See copybara config.

namespace fhe {
namespace rust {
namespace transpiler {

namespace {

using ::xls::netlist::rtl::AbstractModule;
using ::xls::netlist::rtl::AbstractNetRef;
using ::xls::netlist::rtl::NetDeclKind;

using ::fully_homomorphic_encryption::transpiler::CodegenTemplates;
using ::fully_homomorphic_encryption::transpiler::ConstantToValue;
using ::fully_homomorphic_encryption::transpiler::ExtractGateInputs;
using ::fully_homomorphic_encryption::transpiler::ExtractGateOutput;
using ::fully_homomorphic_encryption::transpiler::GateInputs;
using ::fully_homomorphic_encryption::transpiler::GateOutput;
using ::fully_homomorphic_encryption::transpiler::LevelSortedCellNames;
using ::fully_homomorphic_encryption::transpiler::NetRefIdToIndex;
using ::fully_homomorphic_encryption::transpiler::NetRefStem;
using ::fully_homomorphic_encryption::transpiler::ResolveNetRefName;

static auto kCellNameToTfheRsOp =
    absl::flat_hash_map<absl::string_view, absl::string_view>(
        {{"lut3", "lut3"}});

// Used for references to variables inside the scope of the transpiled function.
class InScopeTfheRsTemplates : public CodegenTemplates {
 public:
  InScopeTfheRsTemplates() = default;
  ~InScopeTfheRsTemplates() override = default;

  std::string ConstantCiphertext(int value) const override {
    return value == 0 ? "constant_false" : "constant_true";
  }

  std::string PriorGateOutputReference(absl::string_view ref) const override {
    return absl::StrFormat("temp_nodes[%s]", ref);
  }

  std::string InputOrOutputReference(absl::string_view ref) const override {
    return std::string(ref);
  }
};

// Used to build the constant task data definitions.
class ConstTfheRsTemplates : public CodegenTemplates {
 public:
  ConstTfheRsTemplates(const xlscc_metadata::MetadataOutput& metadata,
                       absl::string_view output_stem)
      : metadata_(metadata), output_stem_(output_stem) {}
  ~ConstTfheRsTemplates() override = default;

  std::string ConstantCiphertext(int value) const override {
    return value == 0 ? "Cst(false)" : "Cst(true)";
  }

  std::string PriorGateOutputReference(absl::string_view ref) const override {
    return absl::StrFormat("Tv(%s)", ref);
  }

  std::string InputOrOutputReference(absl::string_view ref) const override {
    std::vector<std::string> tokens =
        absl::StrSplit(ref, absl::ByAnyChar("[]"));

    if (tokens[0] == output_stem_) {
      return absl::StrFormat("Output(%s)", tokens[1]);
    }

    int arg_index = 0;
    for (auto& param : metadata_.top_func_proto().params()) {
      if (param.name() == tokens[0]) {
        break;
      }
      ++arg_index;
    }
    return absl::StrFormat("Arg(%d, %s)", arg_index, tokens[1]);
  }

 private:
  const xlscc_metadata::MetadataOutput& metadata_;
  absl::string_view output_stem_;
};

const InScopeTfheRsTemplates& GetTfheRsTemplates() {
  static const auto* templates = new InScopeTfheRsTemplates();
  return *templates;
}

absl::StatusOr<std::string> OutputStem(const AbstractModule<bool>& module) {
  // Each bit of each output corresponds to one entry in the `outputs` vector.
  // Hence, to validate we have only one output variable, we count the
  // occurrences of each name stem.
  absl::flat_hash_set<std::string> output_stem_names;
  for (const auto& output : module.outputs()) {
    output_stem_names.insert(NetRefStem(output->name()));
  }

  if (output_stem_names.size() != 1) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Modules with $0 outputs are not supported, names were: $1",
        output_stem_names.size(), absl::StrJoin(output_stem_names, ",")));
  }
  return std::string(*(output_stem_names.begin()));
}

absl::StatusOr<absl::btree_set<int>> ExtractLuts(
    const AbstractModule<bool>& module) {
  absl::btree_set<int> luts;
  for (const auto& cell : module.cells()) {
    XLS_ASSIGN_OR_RETURN(const GateInputs extracted_inputs,
                         ExtractGateInputs(cell.get(), GetTfheRsTemplates()));
    luts.insert(extracted_inputs.lut_definition);
  }
  return luts;
}

absl::StatusOr<int> CoerceIntegralOutputIndex(
    const AbstractNetRef<bool>& netref) {
  if (netref->kind() == NetDeclKind::kWire) {
    return absl::InvalidArgumentError(absl::Substitute(
        "Attempting to treat $0 as an output value, but it is not supported",
        netref->name()));
  }
  return NetRefIdToIndex(netref->name());
}

}  // namespace

absl::StatusOr<std::string> YosysTfheRsTranspiler::Translate(int parallelism) {
  std::pair<std::string, int> gate_ops_and_level_count;
  XLS_ASSIGN_OR_RETURN(gate_ops_and_level_count, BuildGateOps(parallelism));
  std::vector<std::string> run_level_commands;
  auto level_count = gate_ops_and_level_count.second;
  run_level_commands.reserve(level_count);
  for (int level_id = 0; level_id < level_count; ++level_id) {
    run_level_commands.push_back(absl::StrFormat(kRunLevelTemplate, level_id));
  }

  XLS_ASSIGN_OR_RETURN(std::string signature, FunctionSignature());
  XLS_ASSIGN_OR_RETURN(std::string output_stem, OutputStem(module()));

  XLS_ASSIGN_OR_RETURN(absl::btree_set<int> luts, ExtractLuts(module()));
  int num_gates = module().cells().size();

  std::vector<std::string> ordered_params;
  for (auto& param : metadata_.top_func_proto().params()) {
    ordered_params.push_back(param.name());
  }

  XLS_ASSIGN_OR_RETURN(std::string output_assignments, AssignOutputs());
  // Idiomatic in rust to not include return as a statement, but default to
  // returning the last line of a statement block as an expression.
  std::string return_statement =
      output_stem + (module().outputs().size() == 1 ? "[0]" : "");

  return absl::StrReplaceAll(
      kCodegenTemplate,
      {{"$gate_levels", gate_ops_and_level_count.first},
       {"$function_signature", signature},
       {"$ordered_params", absl::StrJoin(ordered_params, ", ")},
       {"$num_luts", absl::StrFormat("%d", luts.size())},
       {"$comma_separated_luts", absl::StrJoin(luts, ", ")},
       {"$total_num_gates", absl::StrFormat("%d", num_gates)},
       {"$output_stem", output_stem},
       {"$num_outputs", absl::StrFormat("%d", module().outputs().size())},
       {"$run_level_ops", absl::StrJoin(run_level_commands, "\n")},
       {"$output_assignment_block", output_assignments},
       {"$return_statement", return_statement}});
}

absl::StatusOr<std::pair<std::string, int>> YosysTfheRsTranspiler::BuildGateOps(
    int parallelism) {
  // If parallelism = 0, it's interpreted as unbounded parallelism,
  // and because the impl code depends on a nonzero value for parallelism,
  // we can upper bound the parallelism by the total number of ops.
  int gate_parallelism =
      parallelism == 0 ? module().cells().size() : parallelism;

  std::vector<std::string> task_blocks;
  XLS_LOG(INFO) << "Generating parallel gate ops with parallelism "
                << gate_parallelism << "\n";

  XLS_ASSIGN_OR_RETURN(std::string output_stem, OutputStem(module()));
  ConstTfheRsTemplates const_templates(metadata_, output_stem);

  XLS_ASSIGN_OR_RETURN(std::vector<std::vector<std::string>> cell_levels,
                       LevelSortedCellNames(module()));
  int codegen_level_id = 0;
  for (int level_index = 0; level_index < cell_levels.size(); ++level_index) {
    auto& level = cell_levels[level_index];
    std::sort(level.begin(), level.end());

    int num_batches = (int)ceil((double)level.size() / gate_parallelism);
    for (int batch_index = 0; batch_index < num_batches; ++batch_index) {
      // All cells in each level are assumed to have the same gate operation
      // name (e.g., they are all lut3 or all xnor), and if this is not the
      // case it is an error with the yosys techmapping step.
      // TODO: assert all cells in the level have the same gate
      // operation or fail.

      int remaining_gates = level.size() - (batch_index * gate_parallelism);
      int batch_bound = std::min(gate_parallelism, remaining_gates);
      std::vector<std::string> tasks;
      for (int index_within_batch = 0; index_within_batch < batch_bound;
           ++index_within_batch) {
        int cell_index = batch_index * gate_parallelism + index_within_batch;
        const auto& cell_name = level[cell_index];

        XLS_ASSIGN_OR_RETURN(const auto& cell, module().ResolveCell(cell_name));

        if (cell->outputs().length() > 1) {
          return absl::InvalidArgumentError(
              "Cells with more than one output pin are not supported.");
        }

        XLS_ASSIGN_OR_RETURN(const GateInputs extracted_inputs,
                             ExtractGateInputs(cell, const_templates));
        XLS_ASSIGN_OR_RETURN(const GateOutput extracted_output,
                             ExtractGateOutput(cell));
        std::string task =
            absl::StrFormat(kTaskTemplate, extracted_output.index,
                            extracted_output.is_output ? "true" : "false",
                            extracted_inputs.lut_definition,
                            absl::StrJoin(extracted_inputs.inputs, ", "));
        tasks.push_back(std::move(task));
      }
      task_blocks.push_back(absl::StrFormat(kLevelTemplate, codegen_level_id,
                                            tasks.size(),
                                            absl::StrJoin(tasks, "\n")));
      ++codegen_level_id;
    }
  }

  return std::pair(absl::StrJoin(task_blocks, "\n"), codegen_level_id);
}

// Statements that copy internal temp_nodes to the output locations.
absl::StatusOr<std::string> YosysTfheRsTranspiler::AssignOutputs() {
  std::vector<std::string> assignments;
  XLS_ASSIGN_OR_RETURN(std::string output_stem, OutputStem(module()));

  for (const auto& [key, value] : module().assigns()) {
    if (key->kind() != NetDeclKind::kOutput) {
      return absl::InvalidArgumentError(
          "Unsupported assign statement assigning to non-output variables.");
    }

    // Fails when the key netref is not a module output. The yosys preprocessing
    // should ensure that all assign statements only operate on the final
    // outputs.
    XLS_ASSIGN_OR_RETURN(const int index, CoerceIntegralOutputIndex(key));
    auto templates = GetTfheRsTemplates();

    std::string var_value;
    if (absl::StrContains(value->name(), "constant")) {
      XLS_ASSIGN_OR_RETURN(int constant_value, ConstantToValue(value->name()));
      var_value = templates.ConstantCiphertext(constant_value);
    } else {
      XLS_ASSIGN_OR_RETURN(var_value, ResolveNetRefName(value, templates));
    }
    assignments.push_back(
        absl::StrFormat(kAssignmentTemplate, output_stem, index, var_value));
  }

  // Since map iteration order is arbitrary, and the assignments to outputs
  // are supposed to be unique, we can sort the output and get a deterministic
  // output program.
  std::sort(assignments.begin(), assignments.end());

  // Every Yosys netlist input to this codegen step has exactly one output
  // declared in the netlist, suppose it's called `out`. At this point in the
  // program, all output values are assigned to `out`, and `out` represents
  // the concatenation of possibly multiple outputs in a single bit vector.
  // This primarily happens if the input C++ code has a return value and in/out
  // params.
  //
  // The precise rules for how multiple outputs are concatenated into a single
  // bit vector is unclear (the order does not appear to be consistent), but
  // it seems to always be true that the return value of the function comes
  // last, if present.
  //
  // The existing rules for copying `out` to the various output parameters is
  // specified by
  // //third_party/fully_homomorphic_encryption/transpiler/yosys_cleartext_runner.cc
  // We are adapting that code, with the understanding that we are migrating to
  // MLIR, and when we do that we will use a different interchange format for
  // Yosys that has a more precise specification (RTLIL, for example).

  // The remaining output wires in the netlist follow the declaration order of
  // the input wires in the verilog file.  Suppose you have the following
  // netlist:
  //
  // module foo(a, b, c, out);
  //     input [7:0] c;
  //     input a;
  //     output [7:0] out;
  //     input [7:0] b;
  //
  // Suppose that in that netlist input wires a, b, and c represent in/out
  // parameters in the source language (i.e. they are non-const references in
  // C++).
  //
  // In this case, the return values of these parameters will be splayed out in
  // the output in the same order in which the input wires are declared, rather
  // than the order in which they appear in the module statement.  In other
  // words, at this point out will have c[0], c[1], ... c[7], then it will have
  // a, and finally it will have b[0], ..., b[7].
  //
  // However, the inputs to the runner follow the order in the module statement
  // (which in turn mirrors the order in which they are in the source language.)
  // Therefore, we have to identify which parts of the output wires correspond
  // to each of the input arguments.
  size_t output_index = 0;
  for (auto input : module().inputs()) {
    // Start by pulling off the first input net wire.  Following the example
    // above, it will have the name "c[0]".  (When we get to the single-wire "a"
    // next, the name will simply be "a".).
    std::vector<std::string> name_and_idx = absl::StrSplit(input->name(), '[');
    // Look for the non-indexed name ("c" in the example above) in the list of
    // function arguments, which we can access from the metadata.
    auto found = std::find_if(
        metadata_.top_func_proto().params().cbegin(),
        metadata_.top_func_proto().params().cend(),
        [&name_and_idx](const xlscc_metadata::FunctionParameter& arg) {
          return arg.name() == name_and_idx[0];
        });
    // We must be able to find that parameter--failing to is a bug, as we
    // autogenerate both the netlist and the metadata from the same source file,
    // and provide these parameters to this method.
    XLS_CHECK(found != metadata_.top_func_proto().params().cend());

    if (found->is_reference() && !found->is_const()) {
      // Get the bit size of the argument (e.g., 8 since "c" is defined to be a
      // byte.)
      //
      // In a simplification from the yosys_cleartext_runner, we assume all
      // inputs are integers. No structs, bools, or arrays in cleartext.
      int arg_size = found->type().as_int().width();

      // Now read out the index of the parameter itself (e.g., the 0 in "c[0]").
      size_t param_bit_idx = 0;
      if (name_and_idx.size() == 2) {
        absl::string_view idx = absl::StripSuffix(name_and_idx[1], "]");
        XLS_CHECK(absl::SimpleAtoi(idx, &param_bit_idx));
      }
      // The i'th parameter subscript must be within range (e.g., 0 must be less
      // than 8, since c is a byte in out example.)
      XLS_CHECK(param_bit_idx < arg_size);
      assignments.push_back(
          absl::StrFormat(kAssignmentTemplate, name_and_idx[0], param_bit_idx,
                          GetTfheRsTemplates().InputOrOutputReference(
                              module().outputs()[output_index]->name())));
      output_index++;
    }
  }

  // Some output has yet to be converted to an outparam, and hence it must be
  // the final return value. If no outparam has been seen, then we can skip this
  // step entirely because the output is exactly the final return value.
  // Otherwise, we have to remove the bits from the `out` vector that were
  // copied to outparams, which is the slice starting from the current
  // `output_index` and going to the end of the module's outputs.
  if (output_index > 0 && output_index < module().outputs().size()) {
    std::vector<std::string> out_stem_and_idx =
        absl::StrSplit(module().outputs()[output_index]->name(), '[');
    absl::string_view out_idx = absl::StripSuffix(out_stem_and_idx[1], "]");
    int converted_out_idx = 0;
    XLS_CHECK(absl::SimpleAtoi(out_idx, &converted_out_idx));
    assignments.push_back(absl::StrFormat(kSplitTemplate, out_stem_and_idx[0],
                                          out_stem_and_idx[0],
                                          converted_out_idx));
  }

  return absl::StrJoin(assignments, "\n");
}

absl::StatusOr<std::string> YosysTfheRsTranspiler::FunctionSignature() {
  // Each entry in module.inputs() corresponds to one bit of an input, though
  // many inputs may share the same stem. However, if there is only a single
  // bit in the input, it will be referred to by just the stem (e.g., `value`
  // vs `value[3]`), and it requires a different type in the function
  // signature: a single value instead of a list of values. We count the
  // occurrences of the stem (`value` for `value[3]`) in order to determine if
  // it's a single bit or multi-bit input.
  absl::flat_hash_map<std::string, int> input_stem_counts;
  for (const auto& input : module().inputs()) {
    input_stem_counts[NetRefStem(input->name())] += 1;
  }

  int output_size = module().outputs().size();
  std::string output_type = output_size == 1 ? "Ciphertext" : "Vec<Ciphertext>";

  std::vector<std::string> param_signatures;
  for (const auto& input : module().inputs()) {
    std::string input_stem = NetRefStem(input->name());
    if (!input_stem_counts.count(input_stem)) {
      continue;
    }
    int stem_count = input_stem_counts[input_stem];
    // We want to process this stem exactly once, so that only one argument is
    // added for each possible stem.
    input_stem_counts.erase(input_stem);

    auto param_metadata = std::find_if(
        metadata_.top_func_proto().params().cbegin(),
        metadata_.top_func_proto().params().cend(),
        [&input_stem](const xlscc_metadata::FunctionParameter& arg) {
          return arg.name() == input_stem;
        });
    XLS_CHECK(param_metadata != metadata_.top_func_proto().params().cend());
    const bool is_outparam =
        param_metadata->is_reference() && !param_metadata->is_const();
    absl::string_view rust_ref_type = is_outparam ? "&mut " : "&";

    if (stem_count == 1) {
      param_signatures.push_back(
          absl::StrCat(input_stem, ": ", rust_ref_type, "Ciphertext"));
    } else {
      param_signatures.push_back(
          absl::StrCat(input_stem, ": ", rust_ref_type, "Vec<Ciphertext>"));
    }
  }

  return absl::Substitute(
      "$0($1, server_key: &ServerKey) -> $2",
      google::protobuf::util::converter::ToSnakeCase(module().name()),
      absl::StrJoin(param_signatures, ", "), output_type);
}

}  // namespace transpiler
}  // namespace rust
}  // namespace fhe
