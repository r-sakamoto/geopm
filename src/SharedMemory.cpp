/*
 * Copyright (c) 2015, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include "SharedMemory.hpp"
#include "Exception.hpp"

namespace geopm
{
    SharedMemory::SharedMemory(const std::string &shm_key, size_t size)
        : m_shm_key(shm_key)
        , m_size(size)
    {
        if (size == 0) {
            throw Exception("SharedMemory: Cannot create shared memory region of zero size",  GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        int shm_id = shm_open(m_shm_key.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRWXU | S_IRWXG);
        if (shm_id < 0) {
            throw Exception("SharedMemory: Could not open shared memory with key " + m_shm_key, errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        int err = ftruncate(shm_id, size);
        if (err) {
            (void) close(shm_id);
            (void) shm_unlink(m_shm_key.c_str());
            throw Exception("SharedMemory: Could not extend shared memory to size " + std::to_string(size), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
        if (m_ptr == MAP_FAILED) {
            (void) close(shm_id);
            (void) shm_unlink(m_shm_key.c_str());
            throw Exception("SharedMemory: Could not mmap shared memory region", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = close(shm_id);
        if (err) {
            throw Exception("SharedMemory: Could not close shared memory file", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    SharedMemory::~SharedMemory()
    {
        int err = munmap(m_ptr, m_size);
        if (err) {
            throw Exception("SharedMemory: Could not unmap pointer", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = shm_unlink(m_shm_key.c_str());
        if (err) {
            throw Exception("SharedMemory: Could not unlink shared memory region", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void *SharedMemory::pointer(void)
    {
        return m_ptr;
    }

    SharedMemoryUser::SharedMemoryUser(const std::string &shm_key, size_t size)
        : SharedMemoryUser(shm_key, size, INT_MAX)
    {

    }

    SharedMemoryUser::SharedMemoryUser(const std::string &shm_key, size_t size, unsigned int timeout)
        : m_shm_key(shm_key)
        , m_size(size)
    {
        if (size == 0) {
            throw Exception("SharedMemoryUser: Cannot create shared memory region of zero size",  GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        int shm_id = -1;
        if (timeout) {
            double timeout_usec = 1E6 * timeout;
            struct timeval tv;
            gettimeofday(&tv, NULL);
            double begin_usec = 1E6 * tv.tv_sec + tv.tv_usec;
            double curr_usec = begin_usec;
            for (unsigned int i = 0; shm_id < 0 && curr_usec - begin_usec < timeout_usec; ++i) {
                shm_id = shm_open(shm_key.c_str(), O_RDWR, S_IRWXU | S_IRWXG);
                gettimeofday(&tv, NULL);
                curr_usec = 1E6 * tv.tv_sec + tv.tv_usec;
            }
        }
        else {
            shm_id = shm_open(shm_key.c_str(), O_RDWR, S_IRWXU | S_IRWXG);
        }
        if (shm_id < 0) {
            throw Exception("SharedMemoryUser: Could not open shared memory with key \"" + shm_key + "\"", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
        if (m_ptr == MAP_FAILED) {
            (void) close(shm_id);
            throw Exception("SharedMemoryUser: Could not mmap shared memory region", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        int err = close(shm_id);
        if (err) {
            throw Exception("SharedMemoryUser: Could not close shared memory file", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    SharedMemoryUser::~SharedMemoryUser()
    {
        int err = munmap(m_ptr, m_size);
        if (err) {
            throw Exception("SharedMemory: Could not unmap pointer", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void *SharedMemoryUser::pointer(void)
    {
        return m_ptr;
    }
}
