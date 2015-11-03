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

#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include "gtest/gtest.h"
#include "SharedMemory.hpp"

class MPISharedMemoryTest: public :: testing :: Test
{
    public:
        MPISharedMemoryTest();
        virtual ~MPISharedMemoryTest();
        std::string m_shm_key;
};

MPISharedMemoryTest::MPISharedMemoryTest()
    : m_shm_key("/geopm_shared_memory_test")
{
    std::string shm_key(m_shm_key.c_str());
    shm_unlink(shm_key.c_str());
}

MPISharedMemoryTest::~MPISharedMemoryTest()
{

}

TEST_F(MPISharedMemoryTest, hello)
{
    int rank;
    const char *test_string = "THIS IS THE TEST STRING";
    geopm::SharedMemory *sm = NULL;
    geopm::SharedMemoryUser *smu = NULL;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        sm = new geopm::SharedMemory(m_shm_key, 128);
        strcpy((char *)sm->pointer(), test_string);
    }
    else if (rank == 1) {
        smu = new geopm::SharedMemoryUser(m_shm_key, 128, 5);
        EXPECT_EQ(0, strncmp((char *)smu->pointer(), test_string, strlen(test_string)));
    }
    MPI_Barrier(MPI_COMM_WORLD);
    delete sm;
    delete smu;
}