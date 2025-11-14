for file in tests/**/*.bin; do
    # Get relative path without '.bin' extension
    relpath=${file%.bin}           # tests/task1/test1.bin → tests/task1/test1
    outfile=build/${relpath}.res    # build/tests/task1/test1.res
    
    # Create output directory if it doesn't exist
    mkdir -p $(dirname ${outfile})
    
    echo "Testing: ${file}"
    ./build/release/Final_assignment ${file} ${outfile}
done

for task in tests/*/; do
    taskname=$(basename ${task})
    echo "Comparing ${taskname}..."
    for expected in tests/${taskname}/*.res; do
        basename=$(basename ${expected})
        actual=build/tests/${taskname}/${basename}
        if diff -q ${expected} ${actual} > /dev/null 2>&1; then
            echo "  ✓ ${basename}"
        else
            echo "  ✗ ${basename} - MISMATCH"
        fi
    done
done
