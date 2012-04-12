TEMPLATE = subdirs
CONFIG += debug_and_release
SUBDIRS = CondorC++Adapter PQueueModel PQueue PJobFile PJobCmdLine PJobFileEditor \
    PJobRunner \
    PJobClient

PQueueModel.subdir = PQueueModel
PQueueModel.depends = PJobFile CondorC++Adapter

PQueue.subdir = PQueue
PQueue.depends = PQueueModel

PJobCmdLine.subdir = PJobCmdLine
PJobCmdLine.depends = PJobFile

PJobFileEditor.subdir = PJobFileEditor
PJobFileEditor.depends = PJobFile

PJobRunner.subdir = PJobRunner
PJobRunner.depends = PJobFile

PJobClient.subdir = PJobClient
PJobClient.depends = PJobFile
