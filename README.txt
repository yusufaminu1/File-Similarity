Yusuf Aminu yta12
Tyler Bertrand tjb309


Design Notes:

  WFD Data Structure:
    We used a sorted singly linked list of WordNode structs to represent each
    file's word frequency distribution. Each node stores the word (heap-allocated
    via strdup), its count, and its frequency. The list is kept in lexicographic
    order during insertion so that compute_jsd can use simultaneous iteration
    (a merge-style walk) over both lists without any sorting step at the end.
    Insertion walks the list with a prev pointer to find the correct position,
    incrementing count if the word already exists or splicing in a new node.

  FileSet Data Structure:
    We used a dynamic array (FileSet) of WFD pointers to collect all files.
    It starts with capacity 16 and doubles whenever it is full, using realloc.
    This avoids any fixed upper limit on the number of files.

  Word Buffer in read_file:
    To satisfy the no-maximum-word-length requirement, read_file maintains a
    heap-allocated word buffer that starts at 64 bytes and doubles whenever the
    next character would not fit (with space reserved for the null terminator).

  File I/O:
    We used POSIX open(), read(), and close() as required. Files are read in
    4096-byte chunks. Each byte is classified by is_word_char(); word characters
    are accumulated (lowercased) into the buffer, and any non-word character
    flushes the current buffer as a word via wfd_add_word(). A final flush after
    the read loop handles files that do not end with a delimiter.

  Suffix Check:
    has_suffix uses pointer arithmetic to compare the tail of the filename string
    against the suffix, avoiding a full string copy.

  Sorting:
    After all pairs are computed, qsort is called with a comparator that sorts
    by combined_words in descending order (returns compB - compA).


Testing Plan:

  We created a set of small test files and ran the following scenarios:

  1. Identical files (a.txt and b.txt with the same content):
     Expected JSD = 0.00000. Confirmed output matched.

  2. Completely disjoint files (a.txt vs c.txt with no words in common):
     Expected JSD = 1.00000. Confirmed output matched.

  3. Partial overlap (a.txt vs d.txt, where d.txt is a strict subset):
     Expected 0 < JSD < 1. Got 0.34634.

  4. Single-word file vs a multi-word file:
     Expected JSD = 1.00000 (no shared words). Confirmed.

  5. Two empty files:
     Expected JSD = 0.00000 (both distributions are empty/equal). Confirmed.

  6. Empty file vs a non-empty file:
     Expected JSD = 0.70711 (= sqrt(0.5), the maximum distance from an empty
     distribution to any non-empty one). Confirmed.

  7. File containing numbers and hyphens (e.g., "hello-world 123"):
     Verified that digits and hyphens are treated as word characters and are
     included in the tokenized words.

  8. Three-file comparison (a.txt, b.txt, d.txt):
     Expected 3 pairs sorted by combined word count descending. Confirmed order
     and JSD values were correct.

  9. Directory scan with a subdirectory (testfiles/ containing subdir/nested.txt):
     Verified recursive traversal picks up nested .txt files. Confirmed all
     pairs appeared in output sorted correctly.

  10. Non-.txt files in a directory (ignored.md):
      Verified that files not ending in .txt are skipped during directory scan.

  11. Explicit non-.txt file on the command line:
      Verified that an explicit file argument is always included regardless of
      suffix (no suffix check in process_arg for regular files).

  12. Fewer than 2 files found (one file argument):
      Verified program prints an error to stderr and exits with EXIT_FAILURE.

  13. No arguments:
      Verified program prints usage message to stderr and exits with EXIT_FAILURE.