#!/bin/sh

#
# 863gs Process Monitor
# restart 863gs process if it is killed or not working.
#

WORK_PATH=`pwd`
LOCK_GTSERVER="gt_server"
LOCK_SENSOR="split_hash"

RESTART_GTSERVER="${WORK_PATH}/${LOCK_GTSERVER} -f"
RESTART_SENSOR="${WORK_PATH}/${LOCK_SENSOR} -i eth2,eth3"
#RESTART_SENSOR="${WORK_PATH}/${LOCK_SENSOR} -i dag0:0,dag0:2"

LOG_DAEMON="${WORK_PATH}/p2p-daemon.log"
LOG_GTSERVER="gs.log"
LOG_SENSOR="traffic.log"
BACKUP_PATH="log/"

#path to pgrep command
PGREP="/usr/bin/pgrep"
SLEEP="sleep 60"

# check BACKUP_PATH, if not exists, create it
if [ ! -e "${BACKUP_PATH}" ]; then
  mkdir "${BACKUP_PATH}"
fi

echo "[`date \"+%F %H:%M:%S\"`] === daemon-863gs start ===" > ${LOG_DAEMON}

while [ 1 ]
do
  # find gt_server pid
  $PGREP ${LOCK_GTSERVER} > /dev/null
  if [ $? -ne 0 ] # if process not running
  then
    # restart
    echo "[`date \"+%F %H:%M:%S\"`] restart ${LOCK_GTSERVER}" >> ${LOG_DAEMON}
    if [ -e "${LOG_GTSERVER}" ]; then
      mv ${LOG_GTSERVER} "${BACKUP_PATH}${LOG_GTSERVER}.`date \"+%s\"`"
    fi
    $RESTART_GTSERVER > ${LOG_GTSERVER} &
  fi
  # find sensor
  $PGREP ${LOCK_SENSOR} > /dev/null
  if [ $? -ne 0 ] # if process not running
  then
    echo "[`date \"+%F %H:%M:%S\"`] restart ${LOCK_SENSOR}" >> ${LOG_DAEMON}
    if [ -e "${LOG_SENSOR}" ]; then
      mv ${LOG_SENSOR} "${BACKUP_PATH}${LOG_SENSOR}.`date \"+%s\"`"
    fi
    $RESTART_SENSOR > ${LOG_SENSOR} &
  fi
  $SLEEP
done
