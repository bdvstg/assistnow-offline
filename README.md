# assistnow-offline

Use downloaded mgaoffline.ubx from AssistNow Offline to reduce TTFF. (supports `/dev/ttyUSB0` is the uart device to u-blox receiver)

`assistnow-offline --file=mgaoffline.ubx --query=now --print-raw=true > /dev/ttyUSB0`

### Manual (--help)

    Tool for u-blox .ubx file that download from AssistNow Offline service

    usages:
        assistnow-offline --print=true --data=true --type=true --ver=true
            print all data
        assistnow-offline --print=false --range=true
            only show the range of date
        assistnow-offline --file=my_mgaoffline.ubx
            use my_mgaoffline.ubx instead mgaoffline.ubx
        assistnow-offline --query=2022-12-18,2022-12-25
            only show data that 2022-12-18 to 2022-12-25, both 2022-12-18 and 2022-12-25 included
        assistnow-offline --query=now
            only show data of today (use system time, UTC)
        assistnow-offline --print-raw=true
            show raw data, --print/--data/--type/--ver/--range will be ignore

    options:
        --print, -p
            print datas
        --data, -d
            if --print, print field 'data' or not
        --type, -t
            if --print, print field 'type' or not
        --ver, -v
            if --print, print field 'ver' or not
        --range, -r
            print range or not, the range of date of this file
        --file, -f
            use alternative file instead default mgaoffline.ubx
        --query, -q
            if --print or --print-raw, only show the range of query range, for format pls refer to usages
        --print-raw
            print raw data instead text, for send data to u-blox receiver
            --query will works
            --print/--data/--type/--ver/--range will be ignore
        --help, -h
            hi, it me

    exit code:
        0: normally exit
        2: error, syntax error or can not open file
        13: query nothing
