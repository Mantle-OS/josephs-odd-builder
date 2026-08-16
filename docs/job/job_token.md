# Job Token 




### TokenizerShm

Linux-only shared-memory transport for streaming tokenizer data between processes.

`TokenizerShm` wraps `JobSharedMemory` with a small framed protocol designed for token-generation pipelines.

One endpoint opens the shared-memory ring buffer as a producer and writes framed packets. Another process attaches as a consumer and reads those packets in order.

The transport can carry:

* token ID sequences
* raw UTF-8 text chunks
* end-of-sequence notifications
* reset commands
* heartbeat packets

`TokenizerShm` is an IPC utility rather than a tokenizer algorithm or vocabulary format. It does not perform encoding or decoding itself.

#### TokenShmRole

Identifies the role of one shared-memory endpoint.

* `Producer` — creates and writes to the shared-memory transport
* `Consumer` — attaches to and reads from an existing transport

Write operations are accepted only while opened as a producer, and read operations are accepted only while opened as a consumer.

#### TokenPacketType

Identifies the logical payload carried by one shared-memory packet.

* `Tokens` — sequence of 32-bit token IDs
* `Text` — raw UTF-8 byte string
* `Eos` — end-of-generation or end-of-sequence marker
* `Reset` — request for the consumer to clear or reset its active state
* `Heartbeat` — connection keep-alive packet

`Eos`, `Reset`, and heartbeat-style control messages may use an empty payload.

#### TokenPacketHeader

Fixed-size header preceding every packet payload.

The header contains:

* packet type
* protocol flags
* reserved protocol bits
* payload length in bytes

`TokenPacketHeader` is packed and statically verified to remain exactly eight bytes, making packet framing independent of ordinary compiler structure padding.

#### Producer

`openProducer()` creates and opens a writable shared-memory ring buffer.

The shared-memory key must be non-empty and begin with `/`. The producer also supplies the desired ring-buffer size, which defaults to one MiB.

Before opening, any existing connection owned by the object is closed.

Packet writes are rejected when:

* the object is not connected
* the object is not acting as the producer
* the ring buffer does not currently contain enough free space for the complete packet

The writer checks capacity for both header and payload before beginning the write.

#### Consumer

`openConsumer()` attaches to an existing shared-memory segment in read mode.

The same `/`-prefixed key convention is required.

`readNextPacket()` reads one complete framed message and returns its packet type and payload.

When non-blocking mode is enabled and no complete packet header is currently available, the method returns `std::nullopt`.

#### Packet writing

All public writer helpers eventually pass through `writePacket()`.

`writeToken()` sends one token ID.

`writeTokens()` sends a contiguous sequence of `int32_t` token IDs.

`writeText()` sends the bytes of a text chunk unchanged.

`writeEos()` and `writeReset()` send control packets with no payload.

Token payloads are transmitted using the native in-memory representation of `int32_t`.

#### Packet reading

`readNextPacket()` first reads the fixed eight-byte header and then allocates enough storage for the declared payload.

Payload reads continue until the complete declared byte count has been received.

The returned `Packet` owns its payload storage, so it remains valid independently of subsequent shared-memory reads.

#### Token convenience API

`readTokens()` reads the next framed packet and copies token IDs into caller-provided storage.

* an empty output span returns zero
* a failed or unavailable packet returns `-1`
* a non-token packet returns zero
* a token packet returns the number of token IDs copied

`readAvailableTokens()` repeatedly consumes currently available packets and combines token payloads into one vector.

Collection stops when:

* fewer than one complete packet header remains available
* packet reading fails
* an `Eos` packet is encountered

Non-token packets other than `Eos` are consumed and ignored by this convenience function.

#### Non-blocking mode

`setNonBlocking()` stores the transport policy on `TokenizerShm` and forwards it to the underlying `JobSharedMemory` device.

This allows producer and consumer endpoints to participate in polling or event-driven pipelines without requiring a separate shared-memory wrapper.

#### Lifetime and diagnostics

The destructor calls `close()`, ensuring the underlying shared-memory device is released when the wrapper is destroyed.

`device()` exposes the underlying `JobSharedMemory` object for lower-level diagnostics and integration when the framed tokenizer API is insufficient.

`key()` exposes the currently configured shared-memory key and `isConnected()` reports the state of the underlying device.



### BinaryTokenType
Token classification stored by JOB's native binary vocabulary format.

- `Normal` — ordinary vocabulary token
- `Special` — tokenizer-defined special token
- `Control` — tokenizer control token
- `Byte` — raw-byte fallback token
- `Unused` — vocabulary entry not intended for normal tokenization

The binary token type is part of the serialized format and is later translated into JOB's runtime token metadata.

### BinaryTokenEntry
One vocabulary entry stored in the JOB binary tokenizer format.

Each entry contains:

- `content` — token byte string
- `score` — model score, used by algorithms such as Unigram
- `tokenType` — serialized token classification

As with GGUF, token IDs are implicit from position in the vocabulary vector: entry `N` represents token ID `N`.

### BinaryVocabData
Parsed representation of a JOB binary vocabulary payload.

`BinaryVocabData` contains the complete tokenizer metadata required by the native binary format:

- format version
- tokenizer model family
- byte-fallback configuration
- prefix-space configuration
- canonical special-token IDs
- optional Jinja chat template
- ordered vocabulary entries
- token-to-ID lookup
- ordered BPE merge pairs

It is an intermediate serialized-format representation rather than the runtime `Vocab` itself.

### BinaryVocabReader
Reader for JOB's compact native tokenizer vocabulary format.

`BinaryVocabReader` loads a versioned binary tokenizer payload from disk or memory and converts it into `BinaryVocabData`.

- `loadFromFile()` reads the complete binary file and delegates parsing to the memory loader
- `loadFromMemory(span)` performs bounded parsing over caller-provided bytes
- `loadFromMemory(pointer, size)` provides a raw-buffer convenience overload
- `findTokenId()` resolves token text to its serialized token ID
- `findTokenString()` resolves a numeric ID through the ordered vocabulary
- `clear()` restores an empty default state

#### Binary header
Every payload begins with a packed fixed-size header containing:

- magic value
- format version
- tokenizer model type
- tokenizer flags
- vocabulary count
- merge-rule count
- BOS, EOS, UNK, PAD, and MASK token IDs
- chat-template byte length

The magic constant is `0x4A4F4256` (`JOBV`) and the current format version is `1`.

Payloads with an incorrect magic value or unsupported version are rejected before any tokenizer data is committed.

#### Flags
The version-1 header currently uses two flag bits:

- bit `0x01` — byte fallback enabled
- bit `0x02` — prefix-space behavior enabled

Unused bits remain available for future version-compatible tokenizer flags.

#### Chat template
When `chatTemplateLen` is non-zero, that exact number of bytes is read immediately after the header and stored as the tokenizer's Jinja chat template.

The template is preserved unchanged; parsing and execution belong to the chat-template subsystem.

#### Vocabulary entries
Exactly `vocabSize` entries follow the optional chat template.

Each vocabulary entry is serialized as:

1. 16-bit token-string length
2. token bytes
3. 32-bit floating-point score
4. 8-bit token type

Token IDs are assigned implicitly by entry order.

While loading, the reader also constructs a token-to-ID hash table for reverse lookup.

#### BPE merges
Exactly `mergesSize` merge rules follow the vocabulary.

Each rule contains:

1. 16-bit left-token string length
2. left-token bytes
3. 16-bit right-token string length
4. right-token bytes

Merge ordering is preserved exactly, so vector position retains the BPE rank required when constructing `BpeAlgo`.

#### Bounded memory parsing
The internal `BufferReader` maintains a cursor over a caller-provided byte span.

Before every scalar or string read, it verifies that the requested number of bytes remains in the input buffer. Truncated headers, token entries, strings, scores, token types, and merge rules therefore cause parsing to fail rather than reading past the supplied memory.

Scalar values are copied with `std::memcpy`, avoiding alignment requirements on the input buffer.

#### Transactional loading
Parsing is performed into a temporary `BinaryVocabData` instance.

`m_data` is replaced only after the entire header, template, vocabulary, and merge table have been read successfully.

A malformed or truncated payload therefore never leaves partially parsed tokenizer state visible through the reader.

#### Lookup behavior
`findTokenId()` returns `std::nullopt` when a token string is absent.

`findTokenString()` returns `std::nullopt` for negative or out-of-range IDs.

No unknown-token substitution is performed by the reader.

### GgufTokenType
Token classification stored by the GGUF tokenizer metadata.

The numeric values correspond to the token-type values serialized by GGUF tokenizer vocabularies.

- `Normal` — ordinary vocabulary token
- `Unknown` — unknown-token entry
- `Control` — tokenizer control token
- `UserDefined` — user-defined vocabulary token
- `Unused` — vocabulary entry not intended for normal tokenization
- `Byte` — raw-byte fallback token

`GgufTokenType` is kept separate from the runtime `TokenType` while GGUF data is being parsed. Runtime tokenizer construction can later translate the serialized representation into JOB's internal vocabulary types.

### GgufTokenEntry
Parsed representation of one GGUF vocabulary entry.

Each entry preserves:

- `text` — the serialized token spelling
- `score` — the model-provided token score when available
- `type` — the serialized GGUF token classification

Token IDs are implicit from position in `GgufTokenizerData::vocab`: the entry at index `N` represents token ID `N`.

### GgufTokenizerData
Tokenizer metadata extracted from a GGUF container.

`GgufTokenizerData` is an intermediate representation used while importing a tokenizer. It is not itself JOB's runtime `Vocab`.

It stores:

- tokenizer model name and mapped tokenizer family
- pre-tokenizer identifier
- ordered vocabulary entries
- token-to-ID lookup
- BPE merge rules
- special-token IDs
- resolved special-token strings
- Jinja chat template
- BOS/EOS and prefix-space preprocessing flags

The vocabulary vector preserves GGUF token-ID ordering directly.

### GgufTokenizerReader
Reader for tokenizer metadata embedded in GGUF model files.

`GgufTokenizerReader` extracts tokenizer configuration from `JobGguf` and converts it into `GgufTokenizerData`.

It can operate from:

- a GGUF file on disk
- a raw memory buffer
- a `std::span<const std::byte>`
- an already opened `ggml::JobGguf`

All loading routes ultimately pass through `loadFromGguf()`, keeping GGUF tokenizer interpretation in one implementation path.

- `loadFromFile()` opens and validates a GGUF file before parsing tokenizer metadata
- `loadFromMemory()` opens GGUF data directly from caller-owned memory
- `loadFromGguf()` extracts tokenizer metadata from an existing GGUF object
- `findTokenId()` resolves token text to its serialized token ID
- `findTokenString()` resolves an ID through the ordered vocabulary
- `clear()` resets all parsed tokenizer state

#### Model metadata
The reader uses `tokenizer.ggml.model` to preserve the serialized tokenizer model name and map it to the tokenizer-family classification used by the format layer.

The optional `tokenizer.ggml.pre` value records the model's pre-tokenizer family, such as LLaMA 3 or Qwen-style preprocessing.

The tokenizer model name and pre-tokenizer are intentionally retained independently: the vocabulary algorithm and the text preprocessing strategy are separate concerns.

#### Special tokens
GGUF stores special-token identities as numeric vocabulary IDs.

The reader recognizes BOS, EOS, unknown, padding, classification, separator, and mask token IDs.

After the vocabulary has been loaded, known IDs are resolved back into their textual token spellings where appropriate. This preserves both forms:

- numeric ID for direct runtime registration
- textual representation for inspection and format conversion

Unassigned special-token IDs remain `-1`.

#### Vocabulary
`tokenizer.ggml.tokens` is required.

Its array order defines token IDs directly, so each string is inserted into `vocab` at the corresponding index and into the token-to-ID lookup table.

Optional `tokenizer.ggml.scores` metadata is applied to the corresponding vocabulary entries.

Optional `tokenizer.ggml.token_type` metadata supplies the serialized classification for each token.

If optional score or type arrays contain fewer entries than the vocabulary, only the overlapping range is applied and remaining entries retain their defaults.

#### BPE merges
When `tokenizer.ggml.merges` is present, merge strings are preserved in serialized order and split into `(left, right)` token-text pairs.

Their ordering therefore retains the BPE ranking needed when the parsed representation is later converted into `BpeAlgo::MergeRule` entries.

The reader does not resolve merge text into runtime token IDs itself.

#### Tokenizer flags
The reader recognizes GGUF metadata controlling:

- automatic BOS insertion
- automatic EOS insertion
- leading-space prefix behavior

Default values are retained when those keys are absent.

#### Chat templates
`tokenizer.chat_template` is read directly from GGUF when present.

The reader stores the template unchanged; parsing and execution belong to the chat-template subsystem rather than the GGUF format layer.

#### Lookup behavior
`findTokenId()` returns `std::nullopt` when a token spelling is absent.

`findTokenString()` returns `std::nullopt` for negative or out-of-range token IDs.

Neither lookup substitutes the model's unknown token automatically.

### HfModelType

Tokenizer model category reported by Hugging Face tokenizer metadata.

* `BPE`
* `WordPiece`
* `Unigram`
* `WordLevel`
* `Unknown`

`hfModelTypeToString()` and `stringToHfModelType()` provide allocation-free conversion between the enum and the exact model-type names used in tokenizer JSON.

`HfModelType` represents the source format's model classification. It remains separate from JOB's `TokenizerAlgorithm` so parsing Hugging Face metadata does not itself select or construct a runtime algorithm implementation.

### HfAddedToken

Parsed representation of one entry from Hugging Face's `added_tokens` array.

Each entry can preserve:

* its explicitly assigned token ID
* token content
* whether it is special
* single-word matching behavior
* left- and right-whitespace stripping behavior
* whether normalizer processing applies to the token

Only added-token entries containing both a non-negative ID and non-empty content are retained by the reader.

### HfTokenizerData

Format-neutralized data extracted from Hugging Face tokenizer files.

`HfTokenizerData` is the reader's output state rather than the runtime `Vocab` itself.

It contains:

* tokenizer model type
* token strings and their scores
* token-to-ID lookup
* BPE merge pairs
* added-token metadata
* named special-token strings
* the model's Jinja chat template
* byte-fallback and prefix-space configuration
* BOS/EOS insertion settings
* tokenization-space cleanup configuration

For ID-based Hugging Face vocabularies, the `vocab` vector is indexed directly by model token ID. Unigram vocabularies instead arrive as ordered `[token, score]` entries and are assigned sequential IDs while being parsed.

### HfTokenizerReader

Reader for Hugging Face tokenizer JSON and associated configuration.

`HfTokenizerReader` translates Hugging Face's serialized tokenizer representation into `HfTokenizerData`. It does not perform tokenization itself and does not construct JOB's runtime `Vocab` or algorithm objects.

* `loadFromFile()` reads `tokenizer.json` and an optional `tokenizer_config.json`
* `loadFromMemory()` performs the same parsing from caller-provided buffers
* `loadTokenizerJson()` parses the tokenizer model, vocabulary, merge rules, added tokens, and tokenizer-level configuration
* `loadTokenizerConfigFile()` / `loadTokenizerConfigJson()` apply model configuration such as special tokens, chat templates, and BOS/EOS behavior
* `findTokenId()` resolves parsed token text to its Hugging Face ID
* `findTokenString()` performs ID-to-text lookup through the parsed vocabulary
* `clear()` restores a fresh `HfTokenizerData` state

Both complete loading entry points call `clear()` first, preventing state from a previously loaded tokenizer from leaking into a new model.

#### tokenizer.json

`loadTokenizerJson()` first parses the document without throwing JSON parse exceptions.

Added tokens are collected before the model vocabulary is processed so that their explicit IDs can later be reconciled into the same vocabulary address space.

The reader accepts a chat template either as a single string or as an array of named templates. When multiple templates are present, a template named `"default"` is preferred; otherwise the first usable template becomes the current template.

Tokenizer-level preprocessing metadata currently includes byte fallback and prefix-space settings.

The required `"model"` object supplies the tokenizer type, model-specific byte-fallback setting, unknown token, vocabulary, and BPE merge rules.

#### Vocabulary forms

Hugging Face serializers use different vocabulary structures depending on tokenizer family.

For object-style vocabularies:

`"token text" -> numeric token ID`

the reader sizes the vector to accommodate the supplied IDs and stores each token directly at its model-assigned position.

For Unigram-style vocabularies:

`[token string, token score]`

the reader preserves the score and assigns IDs according to array order.

A corresponding token-to-ID hash table is populated in either case.

After the model vocabulary is loaded, explicit added tokens are written into their assigned ID positions as well. This allows added and special tokens living outside the original model vocabulary range to remain addressable by their serialized IDs.

#### BPE merges

BPE merge entries are read in serialized order.

Each merge string is divided at its first space into a left and right token spelling and stored as a pair. The vector order therefore preserves the merge ranking needed later when `BpeAlgo` is configured.

The reader does not resolve those strings into `TokenId` merge rules itself; that belongs to the runtime tokenizer construction stage.

#### tokenizer_config.json

The optional tokenizer configuration can provide or override:

* chat template
* BOS token
* EOS token
* unknown token
* padding token
* classification token
* separator token
* mask token
* automatic BOS insertion
* automatic EOS insertion
* tokenization-space cleanup behavior

Special-token values may be represented either directly as strings or as objects containing a `"content"` field; `extractTokenString()` handles both representations.

#### Chat-template fallback

When loading from the filesystem, `HfTokenizerReader` has one additional source for chat formatting.

If neither parsed JSON file produced a chat template, it checks for `chat_template.jinja` beside `tokenizer.json` and uses the complete contents of that file when present.

This fallback is filesystem-specific; `loadFromMemory()` has no sibling-file concept and therefore only uses templates present in its supplied JSON buffers.

#### Lookup behavior

`findTokenId()` returns `std::nullopt` when a token spelling is absent.

`findTokenString()` returns `std::nullopt` for negative or out-of-range IDs.

The reader intentionally exposes these as optional queries rather than substituting an unknown-token ID: loading metadata and deciding tokenizer fallback policy are separate responsibilities.



### WordpieceAlgo

Greedy WordPiece tokenizer using separate prefix tries for initial and continuation subwords.

`WordpieceAlgo` implements the chunk-level `ITokenAlgo` interface for WordPiece vocabularies such as those using the conventional `##` continuation marker.

Tokenization proceeds left to right. The first piece of a chunk is selected from the root vocabulary, while subsequent pieces are selected from vocabulary entries carrying the configured continuation prefix. At each position the longest available piece is chosen.

* `rebuildTries()` rebuilds the initial- and continuation-piece prefix indexes from the active vocabulary
* `setContinuationPrefix()` changes the continuation marker and rebuilds both tries
* `continuationPrefix()` returns the active continuation marker
* `setMaxInputCharsPerWord()` / `maxInputCharsPerWord()` control the maximum accepted input chunk length
* `encodeChunk()` greedily segments one chunk into WordPiece token IDs
* `decodeTokens()` reconstructs token text while removing continuation prefixes
* `type()` identifies the implementation as `TokenizerAlgorithm::WordPiece`

#### Root and continuation tries

The vocabulary is divided into two prefix indexes when `rebuildTries()` runs.

Tokens beginning with the configured continuation prefix are stored in `m_continuationTrie` after that prefix has been removed. All other vocabulary entries are stored unchanged in `m_rootTrie`.

With the default `##` prefix, for example:

`"play"` → root trie as `"play"`

`"##ing"` → continuation trie as `"ing"`

The original vocabulary token ID is retained in either case, so trie matching does not modify the model's actual token IDs or vocabulary strings.

#### Greedy segmentation

`encodeChunk()` begins at byte offset zero and repeatedly performs longest-prefix matching.

The first piece is searched in the root trie. Once at least one piece has been consumed, all subsequent pieces are searched in the continuation trie.

Each successful match immediately becomes part of the result; WordPiece does not compare alternate complete segmentations or use token scores.

For an input such as `playing`, a vocabulary containing `play` and `##ing` can therefore produce:

`play` + `##ing`

while the continuation trie itself searches for the stripped text `ing`.

#### Unknown words

WordPiece requires the complete input chunk to be segmentable.

If any position cannot be matched, the partial segmentation is discarded and the entire chunk is represented by the configured unknown token when one exists.

Likewise, a chunk exceeding `maxInputCharsPerWord()` is immediately represented by the unknown token rather than attempting segmentation.

If no unknown token is configured, these cases return no tokens.

#### Decoding

`decodeTokens()` resolves token IDs through the shared vocabulary.

Ordinary root pieces are copied as stored. When a vocabulary piece begins with the configured continuation prefix, that prefix is removed before the remaining bytes are appended, reversing the representation used during WordPiece encoding.

`<0xNN>` byte-fallback tokens are decoded directly to their raw byte values.

Invalid token IDs and empty vocabulary pieces are ignored.


### UnigramAlgo

Score-based Unigram tokenizer using trie prefix lookup and Viterbi dynamic programming.

`UnigramAlgo` implements the chunk-level `ITokenAlgo` interface for vocabularies where each token carries an independent model score.

Unlike BPE, which repeatedly merges adjacent symbols according to a ranked merge table, Unigram tokenization considers every vocabulary piece that can begin at each reachable input position and chooses the complete segmentation with the highest accumulated score.

* `rebuildTrie()` rebuilds the prefix index from the active vocabulary
* `encodeChunk()` performs a forward Viterbi search over the input and backtracks the highest-scoring complete token path
* `decodeTokens()` reconstructs bytes from ordinary vocabulary pieces and `<0xNN>` byte-fallback tokens
* `trie()` exposes the current prefix index as read-only state
* `type()` identifies the implementation as `TokenizerAlgorithm::Unigram`

#### Vocabulary trie

`rebuildTrie()` inserts every non-empty, valid vocabulary record into an internal `Trie`.

The trie allows `encodeChunk()` to find every vocabulary token matching the input at a given byte position without scanning the complete vocabulary.

The configured unknown token score is also cached when the trie is rebuilt. If no UNK token is configured, the implementation uses a default score of `-10.0`.

#### Dynamic-programming lattice

The input is treated as a sequence of byte positions.

A `DpNode` is maintained for every position from zero through the end of the chunk:

* `bestScore` is the highest cumulative score known for reaching that position
* `bestToken` is the token used by that best path
* `bestPrev` is the preceding byte position used for later backtracking

Position zero begins with a score of `0.0`; all other nodes begin unreachable.

For every reachable position, `findAllPrefixes()` returns the vocabulary pieces matching the remaining input. Each candidate advances by its matched byte length and adds that token's vocabulary score to the accumulated path score.

Only a candidate improving the best score for its destination position replaces the existing path.

#### Finite unreachable scores

The Viterbi table uses large finite negative sentinel values rather than negative infinity.

* `kUnreachableScore` initializes unreachable DP nodes
* `kMinValidScore` distinguishes usable states from unreachable ones

This keeps reachability tests stable when the library is compiled with aggressive floating-point optimization such as fast-math, where assumptions involving infinities can otherwise change compiler behavior.

#### Byte fallback

At each reachable input position, the tokenizer also attempts a single-byte fallback path.

The byte is resolved in this order:

1. an exact one-byte vocabulary token
2. the corresponding `<0xNN>` byte token
3. no byte fallback path if neither exists

This gives the dynamic-programming lattice a one-byte continuation when the vocabulary provides one, allowing otherwise unmatched input bytes to remain representable.

If no vocabulary prefix produced a usable continuation and no byte token exists, the configured unknown token can be used instead with an additional penalty.

#### Optimal-path reconstruction

Once the forward search reaches the end of the input, `encodeChunk()` follows each DP node's `bestPrev` link backward to recover the selected tokens.

The reversed backtracking result is then copied into the caller-provided output span.

If no valid path reaches the final byte position, encoding returns zero tokens.

#### Decoding

`decodeTokens()` resolves each token through the shared `Vocab`.

Ordinary token pieces are copied directly into the output byte buffer. Tokens using the `<0xNN>` syntax are decoded back to their original raw byte.

Invalid token IDs and vocabulary entries with empty text are ignored.





### BpeAlgo

Byte Pair Encoding implementation using ranked integer-pair lookup, a linked symbol sequence, and a priority queue.

`BpeAlgo` implements the chunk-level `ITokenAlgo` interface. Input begins as byte-oriented vocabulary tokens, then configured merge rules are repeatedly applied in rank order until no valid merge candidates remain.

* `addMergeRule()` appends one ranked `(left, right) -> newId` merge rule
* `setMergeRules()` replaces the complete merge table and rebuilds its lookup accelerators
* `clearRules()` removes all merge rules and resets the accelerator tables
* `encodeChunk()` converts one pre-tokenized input chunk into BPE token IDs
* `decodeTokens()` reconstructs bytes from vocabulary pieces, including `<0xNN>` byte-fallback tokens
* `rules()` exposes the configured merge table as a read-only span
* `type()` identifies the implementation as `TokenizerAlgorithm::BPE`

The position of a rule in `m_rules` is its BPE rank: lower indices have higher merge priority.

#### MergeRule

One BPE merge operation.

* `left` is the token ID expected on the left side of the pair
* `right` is the token ID expected on the right side
* `newId` is the vocabulary token that replaces the pair

Merge rules operate entirely on token IDs once the initial byte sequence has been mapped into the vocabulary.

#### Initial byte mapping

Before merges begin, every input byte is converted into an initial token ID by `findInitialByteToken()`.

The lookup order is:

1. use a vocabulary entry containing the literal one-byte string when present
2. use the corresponding `<0xNN>` byte-fallback token when present
3. fall back to the raw numeric byte value as the token ID

This allows ordinary byte-level vocabularies and explicit byte-fallback vocabularies to use the same BPE implementation.

When no merge rules are configured, `encodeChunk()` takes a fast path and emits these initial byte tokens directly.

#### Packed pair lookup

General merge candidates are indexed by packing the two 32-bit token IDs into one 64-bit key:

`(uint64_t(left) << 32) | uint32_t(right)`

This allows merge lookup to use a single integer hash key rather than constructing temporary strings or compound textual keys in the encoding hot path.

`m_pairToRank` maps that packed pair directly to its merge rank.

#### Byte-pair accelerator

Pairs whose left and right token IDs are both in the range `0..255` have an additional direct lookup table.

`m_bytePairRank` contains `256 × 256` rank entries, allowing these common pairs to bypass the hash table entirely.

All other token pairs use the packed 64-bit `m_pairToRank` lookup.

#### Symbol sequence

The input sequence is represented by a `Symbol` array.

Each symbol contains:

* `id` — its current token ID
* `prev` — the previous active symbol
* `next` — the next active symbol

The array itself does not shrink when merges occur. Instead, merging two adjacent symbols updates the left symbol with the merged token ID and splices the right symbol out of the active linked sequence.

This avoids repeatedly erasing elements from the middle of a `std::vector` as the BPE sequence contracts.

#### Merge queue

Every currently mergeable adjacent pair is inserted into a priority queue as a `QueueElement`.

Candidates are ordered first by merge rank and then by source position, so lower-ranked merge rules are applied first with deterministic left-to-right ordering when ranks are equal.

After a merge, only the newly affected neighboring pairs are reconsidered:

* the pair beginning at the previous active symbol
* the pair beginning at the newly merged symbol

This avoids rescanning the complete token sequence after every merge.

#### Stale candidate handling

Priority-queue entries can become obsolete when an earlier merge changes their symbols or adjacency.

Rather than removing arbitrary elements from the heap, `encodeChunk()` validates each candidate when it reaches the top:

* the left symbol must still contain the recorded left token ID
* its current neighbor must still exist
* that neighbor must still contain the recorded right token ID
* the referenced merge rank must still be valid

Invalidated entries are simply discarded.

This keeps queue maintenance inexpensive while preserving correct merge ordering.

#### Decoding

`decodeTokens()` resolves each token ID through the shared `Vocab`.

Ordinary vocabulary pieces are copied directly into the caller-provided byte buffer. Pieces matching the `<0xNN>` fallback syntax are decoded back to their original raw byte instead of emitting the six-character token spelling.

Invalid token IDs and vocabulary entries with empty text are skipped.

Like the base `ITokenAlgo` interface, encoding and decoding write into caller-owned spans and return the number of token IDs or bytes actually written.




### ITokenAlgo
Common interface for tokenizer algorithm implementations.

`ITokenAlgo` defines the chunk-level encode/decode contract shared by BPE, Unigram, WordPiece, and other tokenizer algorithms.

The algorithm layer owns vocabulary segmentation and token reconstruction only. Higher-level concerns such as Unicode preprocessing, regex pre-tokenization, BOS/EOS insertion, special-token parsing, and chat-template rendering are handled outside this interface.

- `type()` identifies the concrete `TokenizerAlgorithm`
- `encodeChunk(chunk, outTokens)` encodes one pre-tokenized text chunk into caller-provided token storage
- `decodeTokens(tokens, outBuffer)` reconstructs token text into caller-provided byte storage
- allocating overloads of both operations provide convenience wrappers around the span-based implementations
- `vocab()` exposes the vocabulary currently used by the algorithm
- `setVocab()` replaces that vocabulary while preserving the algorithm object

#### Vocabulary ownership
Each algorithm holds a shared `Vocab`.

This allows the tokenizer façade and active algorithm implementation to reference the same vocabulary state without duplicating token records, lookup tables, or special-token metadata.

Replacing the vocabulary with `setVocab()` changes the data used by subsequent encode and decode operations.

#### Non-allocating interface
The primary virtual interface is span-based.

`encodeChunk()` writes token IDs into a caller-provided `std::span<TokenId>` and returns the number written. `decodeTokens()` similarly writes reconstructed bytes into caller-provided character storage and returns the byte count.

These operations do not own their output storage, allowing higher-level tokenizer code to reuse buffers across repeated operations.

#### Convenience interface
The allocating overloads build temporary storage, invoke the span-based implementation, then shrink the result to the number of values actually written.

The encode helper initially reserves enough storage for roughly twice the input byte count plus a small margin. The decode helper allocates an estimated sixteen bytes per token plus additional headroom.

These sizes are implementation conveniences rather than part of the tokenizer algorithm contract.


### UnicodeNormalizer
UTF-8 validation, code-point conversion, whitespace cleanup, and SentencePiece-style space normalization.

`UnicodeNormalizer` provides the text-normalization primitives needed before vocabulary segmentation without depending on an external Unicode library.

It does not currently perform canonical Unicode normalization such as NFC, NFD, NFKC, or NFKD. Its normalization behavior is limited to UTF-8 handling and configurable whitespace transformation.

- `isValidUtf8()` validates an entire input byte sequence as UTF-8
- `nextCodepoint()` decodes one UTF-8 code point and advances a caller-owned byte offset
- `encodeCodepoint()` converts a Unicode scalar value back into its UTF-8 byte representation
- `escapeSpaces()` replaces ASCII spaces with the SentencePiece U+2581 `▁` marker and can prepend a dummy marker
- `unescapeSpaces()` converts SentencePiece space markers back to ASCII spaces
- `trimWhitespace()` returns a view with leading and trailing whitespace removed
- `normalize()` applies the configured whitespace-cleaning, dummy-prefix, and SentencePiece-marker behavior

#### UTF-8 validation
`nextCodepoint()` implements UTF-8 decoding directly over the input bytes.

It accepts:

- one-byte ASCII
- valid two-byte sequences
- valid three-byte sequences
- valid four-byte sequences through U+10FFFF

The decoder rejects malformed continuation bytes, overlong encodings, UTF-16 surrogate code points, truncated sequences, and values above the Unicode scalar range.

`isValidUtf8()` repeatedly uses the same decoder, so standalone validation and actual code-point traversal share the same acceptance rules.

#### SentencePiece spaces
`kSentencePieceSpace` is the UTF-8 representation of U+2581 `▁`, the visible space marker commonly used by SentencePiece-derived tokenizers.

`escapeSpaces()` replaces ordinary ASCII spaces with this marker. When dummy-prefix handling is enabled, an input that does not already begin with a space or `▁` receives a leading marker.

`unescapeSpaces()` performs the inverse transformation and can optionally remove one leading dummy marker rather than turning it back into an ordinary space.

#### Options
`UnicodeNormalizer::Options` controls the transformations performed by `normalize()`.

- `addDummyPrefixSpace` adds the tokenizer's leading space convention
- `replaceSpacesWithMarker` converts ASCII spaces to the SentencePiece `▁` marker
- `cleanExtraSpaces` collapses consecutive whitespace into one ASCII space before further processing
- `treatWhitespaceAsToken` records tokenizer whitespace policy but is not currently consumed by `normalize()` itself

When `replaceSpacesWithMarker` is disabled, `addDummyPrefixSpace` inserts a literal ASCII space instead of a SentencePiece marker.

### RegexSplitter

Pre-tokenization splitter that divides input into model-specific byte ranges before the tokenizer algorithm runs.

Many BPE tokenizers do not apply merge rules directly to an entire input string. They first partition the input according to a model-specific regular expression so that words, numbers, whitespace, punctuation, contractions, and line endings enter the merge algorithm as separate chunks.

`RegexSplitter` provides that preprocessing stage while returning views into the original input rather than copying each resulting chunk.

* `setPattern()` selects one of the built-in GPT-2, GPT-4, LLaMA 3, or Qwen 2 pre-tokenization patterns
* `setCustomPattern()` installs a caller-provided regular expression
* `split(text, outChunks)` appends matching and non-matching regions to caller-provided storage
* `split(text)` provides a convenience overload returning a new vector
* `patternType()` identifies the selected built-in or custom pattern
* `patternString()` exposes the expression currently configured
* `isValid()` reports whether the configured expression compiled successfully
* `SplitPattern::None` disables pre-tokenization and causes the complete input to be returned as one chunk

Returned chunks are `std::string_view` objects referencing the original input. The caller must therefore keep the source text alive for as long as any returned chunk is used.

#### SplitPattern

Identifies the family of pre-tokenization expression in use.

* `None` — no splitting; preserve the entire input as one chunk
* `GPT2` — GPT-2-style lexical splitting
* `GPT4` — GPT-4-style Unicode-aware splitting
* `LLaMA3` — LLaMA 3-style pre-tokenization
* `Qwen2` — Qwen 2-style pre-tokenization
* `Custom` — caller-supplied expression

The split pattern is separate from `TokenizerAlgorithm`: models can use the same BPE algorithm while requiring different preprocessing rules.

#### Matching behavior

`split()` scans from the beginning of the supplied byte range and preserves the complete input.

Regex matches become individual chunks, while any bytes between consecutive matches are also emitted rather than discarded. Any remaining unmatched suffix is appended after the final match.

A zero-length regex match is guarded explicitly: one byte is emitted and the scan advances so a malformed or unusual expression cannot trap the splitter in an infinite loop.

If no pattern is configured, or if the configured expression cannot be compiled, the splitter falls back to returning the entire input as one chunk.


### Trie
Byte-oriented prefix tree for fast vocabulary matching.

`Trie` stores token strings as byte paths and maps terminal nodes back to `TokenId` values. It is used where tokenization needs efficient prefix lookup without repeatedly scanning the full vocabulary.

- `insert()` adds a token string and associates its terminal node with a token ID
- `find()` performs exact token-string lookup
- `longestPrefix()` returns the longest vocabulary token matching the beginning of an input string
- `findAllPrefixes()` returns every vocabulary token matching the beginning of an input string, which is useful when constructing Viterbi or other segmentation lattices
- `clear()` removes all entries while preserving a fresh root node
- `nodeCount()` reports the total number of trie nodes, including the root
- `empty()` reports whether the trie contains anything beyond that root node

The trie operates on raw bytes rather than Unicode code points. UTF-8 token strings are therefore stored and matched exactly as their encoded byte sequences.

#### Match
Result of a prefix lookup.

- `id` is the matched vocabulary token ID
- `length` is the number of input bytes consumed by the match

An unmatched result retains `kInvalidToken` and a length of zero.

#### Storage and lookup
Nodes are stored in one contiguous `std::vector` and referenced internally by 32-bit node indices rather than pointers.

Each node stores only:

- an optional terminal `TokenId`
- a sorted vector of outgoing byte edges

Child edges remain ordered by byte value, allowing `findChild()` to use binary search rather than linearly scanning a node's children.

This representation avoids a fixed 256-entry child table for every node while still providing efficient byte lookup for tokenizer vocabularies whose trie nodes usually have relatively few children.


### ByteFallback
Utility for recognizing, parsing, and formatting raw-byte fallback tokens.

Some tokenizer vocabularies represent otherwise unhandled input bytes with synthetic token strings such as `<0x41>`. `ByteFallback` provides the conversion between that textual form and the original byte value.

- `isByteToken()` checks whether a string has the exact `<0xNN>` byte-token form
- `parseByteToken()` converts a valid byte token back into its `uint8_t` value
- `byteToStringView()` converts any byte value to its canonical uppercase `<0xNN>` representation without allocating
- `formatByte()` provides the same conversion as an owning `std::string`

Both uppercase and lowercase hexadecimal digits are accepted while parsing, but generated byte-token strings always use uppercase hexadecimal.

#### Compile-time byte table
All 256 possible byte-token strings are generated into `detail::kByteHexStorage` at compile time.

`byteToStringView()` therefore performs only an indexed lookup into static storage; it does not format numbers or allocate memory at runtime.

The returned `std::string_view` always references static storage and remains valid for the lifetime of the program.

### Vocab
Runtime vocabulary storage and bidirectional token lookup.

`Vocab` owns the `TokenRecord` table used by the tokenizer together with the model's `SpecialTokens` registry.

Token IDs are used directly as indices into a contiguous `std::vector<TokenRecord>`, while a separate hash table maps token text back to its ID. This gives inexpensive ID-to-token access without sacrificing fast string lookup during tokenization.

- `addToken()` appends a token and assigns the next sequential `TokenId`
- `setToken()` installs a token at an explicit ID, growing the record table when necessary
- `findId()` resolves token text to its ID and returns `kInvalidToken` when the text is absent
- `tokenText()`, `tokenScore()`, and `tokenType()` provide lightweight field access by token ID
- `record()` returns the complete `TokenRecord` for callers that need all token metadata
- `records()` exposes the vocabulary as a read-only contiguous span
- `reserve()` preallocates both the record table and text lookup table for vocabularies whose size is known before loading
- `clear()` removes all vocabulary entries and resets the associated special-token registry
- `specialTokens()` exposes the model's canonical and named special-token state

Invalid ID queries are non-throwing: text lookup returns an empty view, score lookup returns `0.0f`, type lookup returns `TokenType::Unknown`, and `record()` returns `nullptr`.

#### StringHash
Transparent string hash used by the vocabulary's text-to-ID lookup table.

`StringHash` accepts `std::string_view` and declares `is_transparent`, allowing `Vocab::findId()` to query the `std::unordered_map<std::string, TokenId>` directly with a string view rather than constructing a temporary owning string.

### SpecialTokens
Registry of canonical and model-defined special token IDs.

`SpecialTokens` keeps the well-known model roles such as BOS, EOS, padding, unknown, and fill-in-the-middle markers in one place while also allowing arbitrary named special tokens to be registered.

- canonical accessors expose BOS, EOS, EOT, PAD, UNK, MASK, PREFIX, SUFFIX, and MIDDLE token IDs
- every canonical setter also registers the ID in the general special-token set
- `registerSpecial()` can associate a model-defined name with an ID and optionally assign that ID one of the canonical `SpecialTokenType` roles
- `isSpecial()` performs an ID-based membership check independent of the token's `TokenRecord::type`
- `findByName()` resolves a registered special-token name to its ID
- `nameById()` performs the inverse lookup and returns an empty view when no name is registered
- `reset()` clears both canonical roles and all custom registrations, restoring every canonical ID to `kInvalidToken`

Canonical IDs and named registrations are intentionally separate concepts. Calling `setBosId()` marks an ID as special and assigns the BOS role, but does not invent a textual name for that token. Conversely, `registerSpecial()` stores a name and can optionally map the same ID onto a canonical role.

The class also keeps a dedicated `m_specialIds` set so callers can test whether an ID is special without searching every canonical field or named-token map.
---
### TokenRecord
Stored representation of one vocabulary entry.

Each record keeps the token's text representation together with the metadata needed by the tokenizer algorithms and decoder.

- `text` is the string representation stored in the vocabulary
- `id` is the discrete token identifier; `kInvalidToken` means the record is not initialized
- `score` stores the model-provided token score used by algorithms such as Unigram
- `type` classifies the token structurally as normal, control, user-defined, byte, unused, or another supported `TokenType`

#### State helpers

- `isValid()` returns true once the record has a real token ID
- `isSpecial()` identifies `Control` and `UserDefined` records as tokens that should be treated specially by the tokenizer
- `isByte()` identifies raw byte fallback tokens
- `isUnused()` identifies vocabulary entries that should not participate in ordinary tokenization

`TokenRecord` deliberately stores structural token metadata only. Higher-level meanings such as BOS, EOS, PAD, or FIM roles are tracked separately through the special-token state rather than being baked into every vocabulary entry.


### job_tokenizer_error.h
Shared tokenizer error codes and their human-readable descriptions.

#### TokenizerErrorCode
Common error classification used across tokenizer loading, encoding, decoding, and vocabulary operations.

- `Success` — operation completed successfully
- `InvalidUtf8` — input contains malformed UTF-8
- `BufferTooSmall` — a caller-provided output span cannot hold the requested result
- `VocabEmpty` — the tokenizer has no usable vocabulary loaded
- `TokenNotFound` — a requested token string does not exist in the vocabulary
- `MergeRuleCorrupted` — a BPE merge rule is malformed or internally inconsistent
- `ContextInvalid` — tokenizer state is not valid for the requested operation
- `IoError` — tokenizer data could not be read or written
- `UnsupportedAlgorithm` — the requested tokenizer algorithm has no supported implementation
- `OutOfMemory` — required memory allocation failed
- `UnknownError` — catch-all for failures that do not map to a more specific tokenizer error

The enum keeps tokenizer-facing error handling independent from lower-level parser, filesystem, allocator, or model-format error mechanisms.

#### errorToString()
`constexpr` conversion from `TokenizerErrorCode` to a static human-readable message.

- allocation-free and `noexcept`
- usable at compile time where the supplied error code is constant
- unknown enum values fall back to `"Unknown tokenizer error"`
- the returned `std::string_view` refers only to static string literals and therefore has no ownership or lifetime requirements

### job_tokenizer_types
Shared token identifiers, classifications, options, and result structures used throughout `job_token`.

#### TokenId
The standard discrete token identifier exposed by the tokenizer API.

- `TokenId` is a signed 32-bit integer
- `kInvalidToken` is `-1`, leaving the non-negative range available for real vocabulary IDs
- this is the normal vocabulary/token-stream representation; it is separate from the optional 21-bit coordinate representation used elsewhere in the library

`kMax21BitTokenId` defines the largest value representable by the library's 21-bit token coordinate scheme: 7 bits each for X, Y, and Z.

#### TokenizerAlgorithm
Identifies the algorithm used to segment input into vocabulary tokens.

- `BPE` — Byte Pair Encoding
- `Unigram` — score-based Unigram tokenization
- `WordPiece` — WordPiece tokenization
- `Motif` — reserved algorithm category for JOB's motif tokenizer work
- `Custom` — implementation-defined tokenizer algorithm

The enum describes tokenizer type independently from whichever file format supplied its vocabulary.

#### TokenType
Structural classification attached to a vocabulary entry.

- `Normal` — ordinary text token
- `Unknown` — unknown/fallback token
- `Control` — tokenizer or model control token
- `UserDefined` — explicitly added user/model token
- `Unused` — vocabulary slot not intended for ordinary tokenization
- `Byte` — token representing a raw byte

`TokenType` describes the nature of the vocabulary entry, not its higher-level special-token purpose.

#### SpecialTokenType
Semantic classification for well-known model tokens.

Includes beginning/end-of-sequence and turn markers, padding and unknown tokens, masking, and prefix/suffix/middle tokens used by fill-in-the-middle models.

A token can therefore have both a structural `TokenType` and a semantic `SpecialTokenType`; for example, an EOS token may be structurally classified as `Control` while its special-token role is `Eos`.

#### TokenizeOptions
Per-call encoding options shared by tokenizer implementations.

- `addBos` requests insertion of the configured beginning-of-sequence token and defaults to `true`
- `addEos` requests insertion of the configured end-of-sequence token and defaults to `false`
- `parseSpecial` controls whether special-token spellings in the input are recognized as special tokens rather than ordinary text
- `defaultMass` supplies the default mass used by token representations that carry a mass value

These defaults make ordinary prompt tokenization begin with BOS while leaving EOS insertion opt-in.

#### TokenSpan
Half-open byte range `[byteBegin, byteEnd)` identifying the source bytes associated with a token.

`length()` returns the number of bytes in the span and safely returns zero if an invalid reversed range is encountered.

The offsets are byte offsets rather than Unicode character indices, so they remain unambiguous for UTF-8 input.

#### TokenizeResult
Detailed result returned when tokenization needs source-location information as well as token IDs.

- `tokens` contains the encoded token stream
- `spans` contains the corresponding source byte ranges

Keeping spans alongside token IDs allows callers to relate generated tokens back to the exact portion of the original byte stream that produced them.