fun ()
{
    echo NEST=1;
    nest ()
    {
        echo this is anoter;
        fun ()
        {
            echo Redefinition;
        };
        echo End of nest;
    };
    echo End of foo;
};
fun
nest
fun
