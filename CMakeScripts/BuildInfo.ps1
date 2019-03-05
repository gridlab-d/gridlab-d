$GIT_OUTPUT = $args[0]

Switch ($args[1])
{
    { "-p", "--pwd" -contains $_ } {
        $RESULT = pwd
    }
    { "-r", "--remote" -contains $_ } {
        $RESULT = ((get-content $GIT_OUTPUT | where { $_ -match "(fetch)" }) -split { $_ -eq "`t" -or $_ -eq " " })[1]
    }
    { "-s", "--status" -contains $_ }
    {
        $RESULT = ((Get-Content $GIT_OUTPUT | ForEach-Object { $_.substring(0, 2) } | Sort-Object -Unique) -join " ").trim(" ")
    }
    { "-b", "--build" -contains $_ }
    {
        ((Get-Content $GIT_OUTPUT) -split { $_ -eq "`t" -or $_ -eq " " })[0]
    }
}
echo $RESULT