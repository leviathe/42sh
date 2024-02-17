switch ()
{
    case $1 in
        a)
            echo Single a
            ;;
        'a*')
            echo ASTAR
            ;;
        a*)
            echo starts with a
            ;;
        *a)
            echo Ends with a
            ;;
        a?)
            echo A and some char
            ;;
        *a*)
            echo Middle a
            ;;
        *)
            echo rest;
    esac
};
switch a
switch ba
switch oof
switch ab
switch aaabbbba
switch baaa
switch bcccda
switch bac
switch '*'a
switch '*'
switch 'a*'
switch 'b*'
switch '**'
