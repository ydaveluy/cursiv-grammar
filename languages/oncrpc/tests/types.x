/* Constants, enums, structs and unions -- the RFC 4506 constructs the
   calculator service does not exercise. */
const MAXUSERNAME = 32;
const MAXFILELEN = -1;

typedef opaque credential<MAXUSERNAME>;
typedef string filename<255>;
typedef unsigned hyper fileid;
typedef int intvec[4];

enum FileKind {
    FILE_REGULAR = 0,
    FILE_DIRECTORY = 1,
    FILE_SYMLINK = 2
};

struct FileHandle {
    fileid id;
    FileKind kind;
    filename path;
    opaque digest[16];
    unsigned hyper timestamps<>;
};

union FileResult switch (FileKind kind) {
    case FILE_REGULAR:
        FileHandle *handle;
    case FILE_DIRECTORY:
    case FILE_SYMLINK:
        void;
    default:
        int errorCode;
};

struct DirEntry {
    filename name;
    FileHandle *target;
    DirEntry *next;
    enum {
        SORT_NAME = 0,
        SORT_SIZE = 1
    } order;
    struct {
        unsigned int low;
        unsigned int high;
    } range;
};

program FILESERVICE {
    version FILESERVICE_V1 {
        FileResult lookup(filename) = 1;
        void remove(filename, credential) = 2;
        intvec stats(void) = 3;
    } = 1;
} = 200001;
