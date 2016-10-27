RETURN_VAL=0
COUNT=0
remainder=0

while [ $RETURN_VAL -eq 0 ]
do
    echo "************"
    echo "RUN $COUNT"
    echo "  cmd: $1"
    echo "************"
    $1 
    RETURN_VAL=$?
    echo "************"
    echo "RETURN VALUE WAS $RETURN_VAL"
    echo "************"
let "COUNT = $COUNT + 1"
done
