# Verify that the program started
echo "Parsing results from test: $1"

if ! grep --quiet "Start execution" $1; then
echo "ERROR: Test program did not start"
exit -1
fi

if ! grep --quiet "Elapsed" $1; then
echo "ERROR: Test program did not terminate"
exit -1
fi

if grep --quiet "ERROR" $1; then
echo "ERROR: Test assertion failed"
exit -1
fi

echo "Test: $1 succeeded"


