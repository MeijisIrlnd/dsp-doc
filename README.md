# DSP Library: Documentation and Tests

This provides tests and doc-generating scripts for Signalsmith Audio's DSP Library.

Signalsmith DSP is included as a submodule in this repo, to clone: 
```
git clone https://github.com/Signalsmith-Audio/dsp-doc.git --recurse-submodules
```

## CMake

All tests and benchmarks can be built by CMake. To configure the repo: 
```
# first run
mkdir build
# always
cd build
cmake -DWERROR=[true/false] ..
```

## Tests

Tests are in `tests/`, and (where possible) they test the actual audio characteristics of the tools (e.g. frequency responses and aliasing levels).

```
make tests
```
or the CMake equivalent:

```
cmake --build . --target all-tests --config [Debug/Release]
```

You can compile/run just specific groups of tests, based on the subfolders in `tests/`, e.g.:

```
make test-delay
```

or the CMake equivalent: 
```
cmake --build . --target delay-tests --config [Debug/Release]
```

The tests are defined with some [simple macros](util/test/tests.h).  Some tests use random numbers, but you can reproduce a run by setting `SEED=` when running.

## Plots

Some of the tests write results to CSV files (as well as verifying them).  Any Python files in the `tests/` (sub-)directory are run if the tests succeed, and these generally plot the CSV results into graphs.

Both the tests and the Python scripts are run from the `out/analysis/` directory.

You'll need SciPy/NumPy and Matplotlib.  These Python plots are being gradually replaced by [my C++ plotting library](https://signalsmith-audio.co.uk/code/plot/). 

There are some supplemental animations, which (if you have `ffmpeg` installed) you can generate with:

```
make analysis-animations
```
