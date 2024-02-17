fun1 ()
{
    if $a; then
    fun2 ()
    {
        echo NO;
    };
    else
    fun2 ()
    {
        echo YES;
    };
    fi;
}
a=false;
fun1
fun2
a=true;
fun1
fun2
a=false;
fun1
fun2
