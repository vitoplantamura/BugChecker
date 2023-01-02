IF EXIST %1 (
    IF EXIST %2 (
        signtool sign /v /f %1 /t http://timestamp.digicert.com /fd sha256 %2
    ) ELSE (
        echo ERROR: Invalid arguments, %2 is not a valid file
        exit 1
    )
) ELSE (
    echo WARNING: Missing %1, skipping signtool
)