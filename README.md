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
  characters at the end of the input and converts the buffer from the mutator's
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

As of commit [13e0fd3](https://github.com/AFLplusplus/AFLplusplus/commit/13e0fd3e1a3767c52bc4243e2132f0fd32579eed),
after running AFL++ for a while, you should observer the following crash:

```
[mutator] Called afl_custom_fuzz with buffer of size 4
  0000  74 65 73 74                                      test
[mutator] Buffer size doesn't match encoded size: expected 1952805752 but got 4
[mutator] According to the encoding, the buffer looks like:
  0000  74 65 73 74 f7 90 87 3f fd ba 1a 29 1f 8e af 3f  test...?...)...?
  0010  15 2b 54 eb 47 aa c0 3f ed a0 ca e7 a5 02 c0 3f  .+T.G..?.......?
  0020  45 e3 07 1a 62 d3 c5 3f 30 b2 12 18 b0 a7 c3 3f  E...b..?0......?
  0030  3c 45 a8 10 d0 ad c2 3f 34 04 c4 8a ac 6f e3 3f  <E.....?4....o.?
  0040  06 00 00 00 00 00 f0 3f 00 00 00 00 00 00 00 00  .......?........
  0050  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  0060  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  0070  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  0080  91 00 00 00 00 00 00 00 80 00 00 00 00 00 00 00  ................
  0090  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  00a0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  00b0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  00c0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  00d0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  00e0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  00f0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
  ... (truncated)

[-] PROGRAM ABORT : Error in custom_fuzz. Size returned: 0
         Location : fuzz_one_original(), src/afl-fuzz-one.c:1874
```

A plain buffer is passed to `afl_custom_fuzz` whereas it expects a buffer
encoded using its internal representation.
