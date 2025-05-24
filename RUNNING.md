# How to build and run the examples
Building and running this repo is non-trivial because shit is kinda broken.

1. `docker build -t google docker/ubuntu-jammy.Dockerfile .`
2. `docker run -it google`
3. `apt update; apt install -y vim` 
4. `bazel build transpiler/examples/...`
    * The above step will fail because `external/rules_python/python/pip_install/extract_wheels/lib/wheel.py` doesn't the metadata of `blinker-1.9.0`. Edit that file (copy the full path from the error in the terminal) and change line 58 to

    ```python
    if metadata.name is None:
        name = "blinker-1.9.0"
    else :
        name = "{}-{}".format(metadata.name.replace("-", "_"), metadata.version)
    ```
5. `bazel build transpiler/examples/...`
    * The above step will fail again because `external/rules_python/python/pip_install/extract_wheels/lib/bazel.py`. Edit that file (copy full path from error in terminal) and note the offending line 453:

    ```python
        contents = generate_build_file_contents(
            name=PY_LIBRARY_LABEL
            if incremental
            else sanitise_name(whl.name, repo_prefix),
            dependencies=sanitised_dependencies,
            whl_file_deps=sanitised_wheel_file_dependencies,
            data_exclude=data_exclude,
            data=data,
            srcs_exclude=srcs_exclude,
            # This fails because whl.metadata.version is None for blinker
            tags=["pypi_name=" + whl.name, "pypi_version=" + whl.metadata.version],
            additional_content=additional_content,
        )
    ```

    Change this to

    ```python
        if whl.metadata.version is None:
            name = "blinker"
            version = "1.9.0"
        else:
            name = whl.name
            version = whl.metadata.version

        contents = generate_build_file_contents(
            name=PY_LIBRARY_LABEL
            if incremental
            else sanitise_name(whl.name, repo_prefix),
            dependencies=sanitised_dependencies,
            whl_file_deps=sanitised_wheel_file_dependencies,
            data_exclude=data_exclude,
            data=data,
            srcs_exclude=srcs_exclude,
            tags=["pypi_name=" + name, "pypi_version=" + version],
            additional_content=additional_content,
        )
    ```

6. `bazel build transpiler/examples/...`
7. Run an example (e.g `bazel-bin/transpiler/examples/cardio/cardio_tfhe_testbench`)
