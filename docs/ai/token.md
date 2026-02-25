# Job AI Token

Tokenization layer for `job::ai`.

Turn raw bytes into “atoms” (`ByteLattice`) and back again.
Different token types just choose different rules for how atoms are formed and when multiple bytes get 
merged into one atom.

- `job::ai::token` = tokenizers + lattice atoms + small helpers
- `job::threads` = used indirectly (CorpusChemist uses the MonteCarlo engine, which uses a ThreadPool)


## The basic shape

### IToken
Common interface:
- bytes → atoms (`ByteLattice`)
- atoms → bytes
- `mutate(seed)` hook for evolution style tokenizers
- optional save/load hooks (some tokenizers keep internal state)

### TokenType
Levels that exist in-tree:
- `Byte`   : raw byte atoms
- `Ascii`  : printable ASCII lane
- `Char`   : UTF-8 aware byte stream turned into lattice atoms
- `Motif`  : merges common byte fragments into single atoms (“molecules”)
- `BPE`    : comparison lane

### TokenFactory
Creates a tokenizer from `TokenType` (Byte/Ascii/Char/Motif).


## ByteLattice

`ByteLattice` is the atom used everywhere.
It’s a small aligned struct (float lanes) that acts like a point in a bounded space,
with a `mass` field used as an extra knob (signal strength / weighting).

There’s a matching “kernel” helper for batch conversion:
- ByteLatticeKernel = fast encode/decode loops over spans (no ownership)

## Unicode lane: `UnicodeLattice`
`UnicodeLattice` for codepoints:
- packs a `char32_t` across 3 axes (7 bits per axis)
- used by the UTF-8 decoding/normalization paths

`UnicodeLatticeKernel` provides batch encode/decode helpers.


## Token implementations

### ByteToken
Lowest level.
Treats the input as raw bytes and maps each byte to one ByteLattice.

No internal state. No mutation behavior.

### AsciiToken
ASCII lane with a fixed vocabulary.
Maps printable ASCII to atoms; non-printable bytes get clamped/mapped into a safe range.

No internal state. No mutation behavior.

### CharToken
UTF-8 aware.
Consumes bytes as a stream, decodes UTF-8, and produces atoms that preserve a
1 byte invariant for the normalized byte form.

This token type is where the “bytes are a stream, not a blob” assumption shows up.

### MotifToken
Molecule lane.
Recognizes multi-byte fragments (motifs) and emits a single atom for a matched fragment.
Fallback behavior emits normal per-byte atoms when no motif matches.

Internal state:
- motif table (id <--> string)
- usage counters per motif id
- a small “immutable zone” of starter motifs

Mutation hook:
- driven by a CorpusChemist (mines candidates from a corpus)
- applies a safety filter before accepting a candidate motif
- evicts low-usage motifs to make room (usage acts like a weak survival signal)

## Helpers

### token_safety_dance.h
“Safe” wrappers that resize vectors before calling encode/decode.
Used to make allocations explicit and keep span calls simple.

### token_concept.h
C++20 concept for “token-like” types:
- encode/decode/mutate with the expected span signatures.

## CorpusChemist
CorpusChemist is a small sampler over a byte corpus:
- given a target length, it randomly probes the corpus
- fingerprints fragments
- keeps a small Top-K of “most frequently observed” candidates
- returns the best candidate substring

This is plugged into MotifToken as the source of new motif candidates.
It uses the Monte Carlo sampling/reduction engine from `job_threads`.
