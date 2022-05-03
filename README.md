# AFLplusplusCustomMutatorPoC

Minimal PoC for a custom AFL++ mutator using an internal representation for
data which doesn't match the one expected by the target.

The mutator's `afl_custom_post_process` is used to convert the data from the
internal representation to the one understood by the target. Indeed, the
[documentation](https://github.com/AFLplusplus/AFLplusplus/blob/stable/docs/custom_mutators.md#custom-mutation)
states the following about `post_process`:

> For some cases, the format of the mutated data returned from the custom
> mutator is not suitable to directly execute the target with this input. For
> example, when using libprotobuf-mutator, the data returned is in a protobuf
> format which corresponds to a given grammar. In order to execute the target,
> the protobuf data must be converted to the plain-text format expected by the
> target. In such scenarios, the user can define the `post_process` function.
> This function is then transforming the data into the format expected by the
> API before executing the target.

# Context

## The target

`target.cpp` is a dummy program which takes a file path as argument, gives it to
its `fuzz` function and unsafely copies the content of the file in a local
buffer of 16 chars:

`memcpy(buf, input.c_str(), input.length());` 

Fuzzing it with AFL++ would intent to find crashes because of this intentional
stack buffer-overflow vulnerability.

## The custom mutator

`mutator.c` is a dummy implementation of a custom mutator which re-defines:

- `afl_custom_init` which allocates memory for the mutator parameters.
- `afl_custom_fuzz` which mutates the given input by adding a random number of
  characters at the end of the input.
- `afl_custom_post_process` which converts the buffer from the mutator's
  internal representation to a plain buffer.
- `afl_custom_deinit` which frees the memory allocated for the mutator
  parameters.

## The expected input format

In this PoC, the input format manipulated by AFL is encoded as the following:

```
| size of the data | data |
```

Example: 

```
0000  00 00 00 04 74 65 73 74                         ....test
```

But the target only expects receiving decoded data, i.e. the data only
(without the size field). That is why the `afl_custom_post_process` function
is needed.

# Setup

## AFL++

You need to clone [AFL++](https://github.com/AFLplusplus/AFLplusplus) in the
`aflplusplus` directory at the root of this repo.
See the [official instructions](https://github.com/AFLplusplus/AFLplusplus/blob/stable/docs/NSTALL.md#linux-on-x86)
to build.

## Compile

Simply run:

```
make
```

## Reproduce

You can fuzz the target using AFL++ and the custom mutator by running:

```
./fuzz
```

# Observation

As of commit [c7bb0a9](https://github.com/AFLplusplus/AFLplusplus/commit/13e0fd3e1a3767c52bc4243e2132f0fd32579eed),
after the initial calibration phase, you should observer the following crash:

```
[mutator] Called afl_custom_fuzz with buffer of size 8
  0000  00 00 00 04 74 65 73 74                          ....test
[mutator] Generated mutated data of size 16
  0000  00 00 00 0c 74 65 73 74 30 31 32 33 34 35 36 37  ....test01234567

[mutator] Called afl_custom_post_process with buffer of size 16
  0000  00 00 00 0c 74 65 73 74 30 31 32 33 34 35 36 37  ....test01234567
[mutator] Generated post-process data of size 12
  0000  74 65 73 74 30 31 32 33 34 35 36 37              test01234567
[target] Reading file at /dev/shm/.cur_input
[target] Read 12 bytes from file
  0000  74 65 73 74 30 31 32 33 34 35 36 37              test01234567
[D] DEBUG: calibration stage 1/8

[mutator] Called afl_custom_post_process with buffer of size 12
  0000  00 00 00 0c 74 65 73 74 30 31 32 33              ....test0123
[mutator] Buffer size doesn't match encoded size: expected 16 but got 12
[mutator] According to the encoding, the buffer looks like:
  0000  00 00 00 0c 74 65 73 74 30 31 32 33 34 35 36 37  ....test01234567

[-] PROGRAM ABORT : Custom_post_process failed (ret: 0)
         Location : write_to_testcase(), src/afl-fuzz-run.c:112
```

The `afl_custom_post_process` function is called twice. The second time, the
wrong size is passed as argument (though the same value is stored in the `buf`
argument). This new size is the value previously returned by the first execution
of `afl_custom_post_process`.
