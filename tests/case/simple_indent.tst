switch () {
case "$*" in
    0)
        echo "0 or less"
        ;;
    (1)
        echo "One"
        ;;
    2)
        echo "2 or more"
        ;;
    *)
        echo "Not an acceptable number"
        ;;
esac
}

switch 42sh is great
switch a
switch 1
switch 0
switch 2
