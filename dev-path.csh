#!/bin/csh

echo $PATH | fgrep `realpath $cwd/../local/bin`
if ( $status != 0 ) then
    setenv PATH `realpath $cwd/../local/bin`:`realpath $cwd/../local/sbin`:${PATH}
endif
