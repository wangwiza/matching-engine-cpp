for testfile in tests/*; do
    ./grader ./engine < "$testfile"
done