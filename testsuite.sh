#/bin/sh

bin_stu="src/42sh"
#bin_stu="bash --posix"
bin_ref="bash --posix"

blue="\033[0;34m"
lblue="\033[1;34m"
#red="\033[0;31m"
red="\033[1;31m"
yellow="\033[1;33m"
norm="\033[0m"
#green="\033[0;32m"
green="\033[1;32m"
cyan="\033[1;36m"
purple="\033[1;35m"

total=0
pass=0
cat_total=0
cat_pass=0
cat_fail=0

out_stu='.out_student'
out_ref='.out_ref'
err_stu='.err_student'
err_ref='.err_ref'

out_aux1='.aux_1'
out_aux2='.aux_2'
out_aux3='.aux_3'

root="tests/"
category=""
path="$root$category"

log="logtests.txt"

printcategorystats()
{
    echo "=================================" #33 '='
    printf "$green"
    printf "Passed:$norm $cat_pass,$red Failed:$norm $cat_fail,$lblue Total:$norm $cat_total\n"
    echo "================================="
    total=$(($total + $cat_total))
    cat_total=0
    pass=$(($pass + $cat_pass))
    cat_pass=0
    cat_fail=0
}

logdebug()
{
    printf "[$lblue DEBUG$norm ] %s\n" "$1"
}

logpass()
{
    printf "[$green PASS$norm ] %s\n" "$1"
}

logfail()
{
    cat_fail=$(($cat_fail + 1))
    #printf "[$red FAIL$norm ] %s\n" "$1"
    printf "[$red FAIL$norm ] "
    #printf "[$red FAIL$norm ] " >> "$log"
}

logbadcode()
{
    printf "[$yellow Bad return code$norm ] "
    printf "$yellow == Bad return code ==$norm " >> "$log"
}

logbadout()
{
    printf "[$yellow Output differed$norm ] "
    printf "$yellow == Output differed ==$norm " >> "$log"
}

logbadredout()
{
    printf "[$yellow Redirected output differed$norm ] "
    printf "$yellow == Redirected output differed ==$norm " >> "$log"
}

lognostderr()
{
    printf "[$yellow Nothing on STDERR$norm ] "
    printf "$yellow == Nothing on STDERR ==$norm " >> "$log"
}

logdesc()
{
    printf "$desc\n"
    printf "%s\n$red -- 42sh out (ret = %d) --$norm\n%s\n$lblue -- Bash out (ret = %d) --$norm\n%s\n" "$desc" "$stu_code" "$(cat $out_stu)" "$ref_code" "$(cat $out_ref)"  >> "$log"
    printf "$purple -- 42sh err --$norm\n%s\n" "$(cat $err_stu)" >> "$log"
    printf "$cyan -- Bash err --$norm\n%s\n" "$(cat $err_ref)" >> "$log"
}

logmemory()
{
    [ -n "$(grep 'AddressSanitizer' $err_stu)" ] \
        && printf "[$lblue MEMORY $norm] ";
    return 0
}

clearredir()
{
    echo -n '' > $out_aux1
}

dotest() #$1 is path to test file
{
    #Function that executes a particular test from a file.
    clearredir
    echo -n '' > $out_aux2

    cat_total=$((cat_total + 1))
    file="$1"
    desc="$file"

    if [ $RANDOM -gt 16384 ]; then
        eval "cat $file | $bin_stu" > $out_stu 2> $err_stu;
        stu_code=$(echo $?);
    else
        eval "$bin_stu $file" > $out_stu 2> $err_stu;
        stu_code=$(echo $?);
    fi

    [ -n "$(grep 'AddressSanitizer' $err_stu)" ] \
        && desc="[$lblue MEMORY $norm] $desc"

    cp $out_aux1 $out_aux2
    clearredir

    eval "cat $file | $bin_ref" > $out_ref 2> $err_ref
    ref_code=$(echo $?)

    [ -n "$(cat $err_stu)" ]\
        && [ "$(cat "$err_stu")" = "" ]\
        && logfail && lognostderr && logmemory && logdesc\
        return 0
    [ "$(diff $out_ref $out_stu)" != "" ]\
        && logfail && logbadout && logmemory && logdesc\
        && return 0
    [ "$(diff $out_aux1 $out_aux2)" != "" ]\
        && logfail && logbadredout && logmemory && logdesc\
        && return 0
    [ "$stu_code" -ne "$ref_code" ]\
        && logfail && logbadcode && logmemory && logdesc\
        && return 0

    logpass "$desc"
    cat_pass=$((cat_pass + 1))
}

dotest_line() #$1 = string; $2 = desc
{
    clearredir
    echo -n '' > $out_aux2

    cat_total=$((cat_total + 1))
    line="$1"
    desc="$1 | $2"

    echo -n "$line" | $bin_stu > $out_stu 2> $err_stu
    stu_code=$(echo $?)

    [ -n "$(grep 'AddressSanitizer' $err_stu)" ] \
        && desc="[$lblue MEMORY $norm] $desc"

    cp $out_aux1 $out_aux2
    clearredir

    echo -n "$line" | $bin_ref > $out_ref 2> $err_ref
    ref_code=$(echo $?)

    [ -n "$(cat $err_stu)" ]\
        && [ "$(cat "$err_stu")" = "" ]\
        && logfail && lognostderr && logmemory && logdesc\
        && return 0
    [ "$(diff $out_ref $out_stu)" != "" ]\
        && logfail && logbadout && logmemory && logdesc\
        && return 0
    [ "$(diff $out_aux1 $out_aux2)" != "" ]\
        && logfail && logbadredout && logmemory && logdesc\
        && return 0
    [ "$stu_code" -ne "$ref_code" ]\
        && logfail && logbadcode && logmemory && logdesc\
        && return 0

    logpass "$desc"
    cat_pass=$((cat_pass + 1))
}

dotest_lines()
{
    #Function that executes all lines of a file as an individual test
    file="$1"

    while IFS='' read -r line; do
        dotest_line "$line" "$file"
    done < "$file"
}

docategorytests()
{
    category=$1
    path="$root$category"

    echo "================================="
    printf "$purple"
    printf "CATEGORY: $category$norm\n"
    echo "================================="
    echo
    for entry in "$path"/*
    do
        case "$entry" in
            *.tst) #Only select test files
                dotest "$entry"
                ;;
            *.ltst)
                dotest_lines "$entry"
                ;;
            *)
                ;;
        esac
    done

    echo; #Printing stats
    printcategorystats;
    echo
}

findcategory() #$1 = category
{
    case "$1" in
        *.sh) #Ignoring testsuite script and Makefile
            ;;
        *Makefile*)
            ;;
        *)
            cat=$(echo "$1" | cut -d '/' -f 2)
            docategorytests "$cat"
            ;;
    esac
}

launchtests()
{
    touch $out_stu $out_ref $err_stu $err_ref $out_aux2
    echo -n "" > "$log"

    echo ""
    logdebug "Launching testsuite..."
    echo ""

    $bin_stu 'uh this doesnt exist' 2> "$out_aux1" #Test for backend error 1
    $bin_stu '-c' 2> "$out_aux1" #Test for backend error 2


    if [ $# -eq 0 ]; then
        for cat in "$root"*;
        do
            findcategory "$cat"
        done
    else
        findcategory "$root$1"
    fi

    logdebug "Printing total stats..."
    echo ""
    cat_total=$total
    cat_pass=$pass
    cat_fail=$(($total - $pass))
    printcategorystats

    echo ""
    logdebug "Testsuite ended successfuly."
    echo ""

    rm $out_stu $out_ref $err_stu $err_ref $out_aux1 $out_aux2
}

launchtests $1
