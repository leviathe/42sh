fun ()
{
    echo toto;
    fun ()
    {
        echo tata;
        fun2 ()
        {
            echo furst;
        }
        echo qi;
    }
    fun2 ()
    {
        echo hworng;
    }
}
