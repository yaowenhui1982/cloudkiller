/**
 All right reserved.
*/
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

int LockMemory(const char* binaryName)
{
    //setuid as root for reset max locked memory resource
    uid_t currentUid = getuid();
    int ret = setuid(0);
    if (0 != ret)
    {   
        std::cerr << "Set user id as root failed:" << std::strerror(errno) << std::endl;
        return -1;
    }
    bool failed = false;
    //get binary file size
    struct stat binaryFileStat;
    ret = stat(binaryName, &binaryFileStat);
    if (0 != ret)
    {
        std::cerr << "Get file size failed:" << std::strerror(errno) << std::endl;
        return -1;
    }
    off_t fileSize = binaryFileStat.st_size;
    
    //set max locked memory resource
    
    int systemPageSize = getpagesize();
    struct rlimit resourceLimit;
    ret = getrlimit(RLIMIT_MEMLOCK, &resourceLimit);
    if (0 != ret)
    {
        std::cerr << "Get current process max locked memory resource limits failed:" 
            << std::strerror(errno) << std::endl;
    }
    int memoryToLock = fileSize % systemPageSize ? 
                       (fileSize + systemPageSize) / systemPageSize * systemPageSize :
                       fileSize;
    resourceLimit.rlim_cur += memoryToLock;
    resourceLimit.rlim_max += memoryToLock;
    ret = setrlimit(RLIMIT_MEMLOCK, &resourceLimit);
    if (0 != ret)
    {
        std::cerr << "Set current process max locked memory resource limits failed:"
            << std::strerror(errno) << std::endl;
    }
    //lock binary to memory
    int fd = open(binaryName, O_RDONLY | O_NOATIME);
    if (fd < 0)
    {   
        LOG_ERROR(sLogger, ("OpenBinaryFileFailed", binaryName)
                ("Error", strerror(errno)));
    }
    else
    {
        void* binaryMemory = mmap(NULL,
                fileSize,
                PROT_EXEC | PROT_READ,
                MAP_SHARED | MAP_NORESERVE | MAP_LOCKED | MAP_POPULATE,
                fd,
                0);
        
        if (MAP_FAILED == binaryMemory)
        {
            std::cerr << "Get locked memory failed:" << std::strerror(errno) << std::endl;
            failed = true;
        }
        else
        {
            //the following line is used to load all the file content to memory;
            //the following read is not efficient, you should replace it with crc32 
            // or other memory calculation methon dependents on cpu hard-ware instruments
            char crc = 0; 
            for(long int index = 0; index < fileSize; ++index)
            {
                crc |= static_cast<char*>(binaryMemory)[index];
            }
        }
        close(fd);
    }
    //the mmaped buffer should not be munmap, which will prevent the binary data cleared from memory
    //munmap(binaryMemory, fileLength);

    //recover user id as origin, the following line should be executed always
    if (0 != setuid(currentUid))
    {
        std::cerr << "Recover user id failed:" << std::strerror(errno) << std::endl;
    }
    return 0;
}

int main
