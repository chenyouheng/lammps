// unit tests for the LAMMPS base class

#include "lammps.h"
#include <mpi.h>
#include <cstdio>  // for stdin, stdout
#include <string>

#include "gtest/gtest.h"

namespace LAMMPS_NS
{
    // test fixture for regular tests
    class LAMMPS_plain : public ::testing::Test {
    protected:
        LAMMPS *lmp;
        LAMMPS_plain() : lmp(nullptr) {
            const char *args[] = {"LAMMPS_test"};
            char **argv = (char **)args;
            int argc = sizeof(args)/sizeof(char *);

            int flag;
            MPI_Initialized(&flag);
            if (!flag) MPI_Init(&argc,&argv);
        }

        ~LAMMPS_plain() override {
            lmp = nullptr;
        }

        void SetUp() override {
            const char *args[] = {"LAMMPS_test",
                                  "-log", "none",
                                  "-echo", "both",
                                  "-nocite"};
            char **argv = (char **)args;
            int argc = sizeof(args)/sizeof(char *);

            ::testing::internal::CaptureStdout();
            lmp = new LAMMPS(argc, argv, MPI_COMM_WORLD);
            std::string output = testing::internal::GetCapturedStdout();
            EXPECT_STREQ(output.substr(0,8).c_str(), "LAMMPS (");
        }

        void TearDown() override {
            ::testing::internal::CaptureStdout();
            delete lmp;
            std::string output = testing::internal::GetCapturedStdout();
            EXPECT_STREQ(output.substr(0,16).c_str(), "Total wall time:");
        }
    };

    TEST_F(LAMMPS_plain, InitMembers)
    {
        EXPECT_NE(lmp->memory, nullptr);
        EXPECT_NE(lmp->error, nullptr);
        EXPECT_NE(lmp->universe, nullptr);
        EXPECT_NE(lmp->input, nullptr);

        EXPECT_NE(lmp->atom, nullptr);
        EXPECT_NE(lmp->update, nullptr);
        EXPECT_NE(lmp->neighbor, nullptr);
        EXPECT_NE(lmp->comm, nullptr);
        EXPECT_NE(lmp->domain, nullptr);
        EXPECT_NE(lmp->force, nullptr);
        EXPECT_NE(lmp->modify, nullptr);
        EXPECT_NE(lmp->group, nullptr);
        EXPECT_NE(lmp->output, nullptr);
        EXPECT_NE(lmp->timer, nullptr);

        EXPECT_EQ(lmp->world, MPI_COMM_WORLD);
        EXPECT_EQ(lmp->infile, stdin);
        EXPECT_EQ(lmp->screen, stdout);
        EXPECT_EQ(lmp->logfile, nullptr);
        EXPECT_GE(lmp->initclock, 0.0);

        EXPECT_EQ(lmp->suffix_enable, 0);
        EXPECT_EQ(lmp->suffix, nullptr);
        EXPECT_EQ(lmp->suffix2, nullptr);

        EXPECT_STREQ(lmp->exename, "LAMMPS_test");
        EXPECT_EQ(lmp->num_package, 0);
        EXPECT_EQ(lmp->clientserver, 0);

        EXPECT_EQ(lmp->kokkos, nullptr);
        EXPECT_EQ(lmp->atomKK, nullptr);
        EXPECT_EQ(lmp->memoryKK, nullptr);
        EXPECT_NE(lmp->python, nullptr);
        EXPECT_EQ(lmp->citeme, nullptr);
        if (LAMMPS::has_git_info) {
            EXPECT_STRNE(LAMMPS::git_commit,"");
            EXPECT_STRNE(LAMMPS::git_branch,"");
            EXPECT_STRNE(LAMMPS::git_descriptor,"");
        } else {
            EXPECT_STREQ(LAMMPS::git_commit,"(unknown)");
            EXPECT_STREQ(LAMMPS::git_branch,"(unknown)");
            EXPECT_STREQ(LAMMPS::git_descriptor,"(unknown)");
        }
    }

    TEST_F(LAMMPS_plain, TestStyles)
    {
        // skip tests if base class is not available
        if (lmp == nullptr) return;
        const char *found;

        const char *atom_styles[] = {
            "atomic", "body", "charge", "ellipsoid", "hybrid",
            "line", "sphere", "tri", NULL };
        for (int i = 0; atom_styles[i] != NULL; ++i) {
            found = lmp->match_style("atom",atom_styles[i]);
            EXPECT_STREQ(found, NULL);
        }

        const char *molecule_atom_styles[] = {
            "angle", "bond", "full", "molecular", "template", NULL };
        for (int i = 0; molecule_atom_styles[i] != NULL; ++i) {
            found = lmp->match_style("atom",molecule_atom_styles[i]);
            EXPECT_STREQ(found, "MOLECULE");
        }

        const char *kokkos_atom_styles[] = {
            "angle/kk", "bond/kk", "full/kk", "molecular/kk", "hybrid/kk", NULL };
        for (int i = 0; kokkos_atom_styles[i] != NULL; ++i) {
            found = lmp->match_style("atom",kokkos_atom_styles[i]);
            EXPECT_STREQ(found, "KOKKOS");
        }
        found = lmp->match_style("atom","dipole");
        EXPECT_STREQ(found,"DIPOLE");
        found = lmp->match_style("atom","peri");
        EXPECT_STREQ(found,"PERI");
        found = lmp->match_style("atom","spin");
        EXPECT_STREQ(found,"SPIN");
        found = lmp->match_style("atom","wavepacket");
        EXPECT_STREQ(found,"USER-AWPMD");
        found = lmp->match_style("atom","dpd");
        EXPECT_STREQ(found,"USER-DPD");
        found = lmp->match_style("atom","edpd");
        EXPECT_STREQ(found,"USER-MESODPD");
        found = lmp->match_style("atom","mdpd");
        EXPECT_STREQ(found,"USER-MESODPD");
        found = lmp->match_style("atom","tdpd");
        EXPECT_STREQ(found,"USER-MESODPD");
        found = lmp->match_style("atom","spin");
        EXPECT_STREQ(found,"SPIN");
        found = lmp->match_style("atom","smd");
        EXPECT_STREQ(found,"USER-SMD");
        found = lmp->match_style("atom","meso");
        EXPECT_STREQ(found,"USER-SPH");
        found = lmp->match_style("atom","i_don't_exist");
        EXPECT_STREQ(found,NULL);
    }

    // test fixture for OpenMP with 2 threads
    class LAMMPS_omp : public ::testing::Test {
    protected:
        LAMMPS *lmp;
        LAMMPS_omp() : lmp(nullptr) {
            const char *args[] = {"LAMMPS_test"};
            char **argv = (char **)args;
            int argc = sizeof(args)/sizeof(char *);

            int flag;
            MPI_Initialized(&flag);
            if (!flag) MPI_Init(&argc,&argv);
        }

        ~LAMMPS_omp() override {
            lmp = nullptr;
        }

        void SetUp() override {
            const char *args[] = {"LAMMPS_test",
                                  "-log", "none",
                                  "-screen", "none",
                                  "-echo", "screen",
                                  "-pk", "omp","2", "neigh", "yes",
                                  "-sf", "omp"
            };
            char **argv = (char **)args;
            int argc = sizeof(args)/sizeof(char *);

            // only run this test fixture with omp suffix if USER-OMP package is installed

            if (LAMMPS::is_installed_pkg("USER-OMP"))
              lmp = new LAMMPS(argc, argv, MPI_COMM_WORLD);
            else GTEST_SKIP();
        }

        void TearDown() override {
            delete lmp;
        }
    };

    TEST_F(LAMMPS_omp, InitMembers)
    {
        EXPECT_NE(lmp->memory, nullptr);
        EXPECT_NE(lmp->error, nullptr);
        EXPECT_NE(lmp->universe, nullptr);
        EXPECT_NE(lmp->input, nullptr);

        EXPECT_NE(lmp->atom, nullptr);
        EXPECT_NE(lmp->update, nullptr);
        EXPECT_NE(lmp->neighbor, nullptr);
        EXPECT_NE(lmp->comm, nullptr);
        EXPECT_NE(lmp->domain, nullptr);
        EXPECT_NE(lmp->force, nullptr);
        EXPECT_NE(lmp->modify, nullptr);
        EXPECT_NE(lmp->group, nullptr);
        EXPECT_NE(lmp->output, nullptr);
        EXPECT_NE(lmp->timer, nullptr);

        EXPECT_EQ(lmp->world, MPI_COMM_WORLD);
        EXPECT_EQ(lmp->infile, stdin);
        EXPECT_EQ(lmp->screen, nullptr);
        EXPECT_EQ(lmp->logfile, nullptr);
        EXPECT_GE(lmp->initclock, 0.0);

        EXPECT_EQ(lmp->suffix_enable, 1);
        EXPECT_STREQ(lmp->suffix, "omp");
        EXPECT_EQ(lmp->suffix2, nullptr);

        EXPECT_STREQ(lmp->exename, "LAMMPS_test");
        EXPECT_EQ(lmp->num_package, 1);
        EXPECT_EQ(lmp->clientserver, 0);

        EXPECT_EQ(lmp->kokkos, nullptr);
        EXPECT_EQ(lmp->atomKK, nullptr);
        EXPECT_EQ(lmp->memoryKK, nullptr);
        EXPECT_NE(lmp->python, nullptr);
        EXPECT_NE(lmp->citeme, nullptr);
        if (LAMMPS::has_git_info) {
            EXPECT_STRNE(LAMMPS::git_commit,"");
            EXPECT_STRNE(LAMMPS::git_branch,"");
            EXPECT_STRNE(LAMMPS::git_descriptor,"");
        } else {
            EXPECT_STREQ(LAMMPS::git_commit,"(unknown)");
            EXPECT_STREQ(LAMMPS::git_branch,"(unknown)");
            EXPECT_STREQ(LAMMPS::git_descriptor,"(unknown)");
        }
    }

    // test fixture for Kokkos tests
    class LAMMPS_kokkos : public ::testing::Test {
    protected:
        LAMMPS *lmp;
        LAMMPS_kokkos() : lmp(nullptr) {
            const char *args[] = {"LAMMPS_test"};
            char **argv = (char **)args;
            int argc = sizeof(args)/sizeof(char *);

            int flag;
            MPI_Initialized(&flag);
            if (!flag) MPI_Init(&argc,&argv);
        }

        ~LAMMPS_kokkos() override {
            lmp = nullptr;
        }

        void SetUp() override {
            const char *args[] = {"LAMMPS_test",
                                  "-log", "none",
                                  "-echo", "none",
                                  "-screen", "none",
                                  "-k", "on","t", "2",
                                  "-sf", "kk"
            };
            char **argv = (char **)args;
            int argc = sizeof(args)/sizeof(char *);

            // only run this test fixture with kk suffix if KOKKOS package is installed
            // also need to figure out a way to find which parallelizations are enabled

            if (LAMMPS::is_installed_pkg("KOKKOS")) {
                ::testing::internal::CaptureStdout();
                lmp = new LAMMPS(argc, argv, MPI_COMM_WORLD);
                std::string output = testing::internal::GetCapturedStdout();
                EXPECT_STREQ(output.substr(0,16).c_str(), "Kokkos::OpenMP::");
            } else GTEST_SKIP();
        }

        void TearDown() override {
            delete lmp;
        }
    };

    TEST_F(LAMMPS_kokkos, InitMembers)
    {
        EXPECT_NE(lmp->memory, nullptr);
        EXPECT_NE(lmp->error, nullptr);
        EXPECT_NE(lmp->universe, nullptr);
        EXPECT_NE(lmp->input, nullptr);

        EXPECT_NE(lmp->atom, nullptr);
        EXPECT_NE(lmp->update, nullptr);
        EXPECT_NE(lmp->neighbor, nullptr);
        EXPECT_NE(lmp->comm, nullptr);
        EXPECT_NE(lmp->domain, nullptr);
        EXPECT_NE(lmp->force, nullptr);
        EXPECT_NE(lmp->modify, nullptr);
        EXPECT_NE(lmp->group, nullptr);
        EXPECT_NE(lmp->output, nullptr);
        EXPECT_NE(lmp->timer, nullptr);

        EXPECT_EQ(lmp->world, MPI_COMM_WORLD);
        EXPECT_EQ(lmp->infile, stdin);
        EXPECT_EQ(lmp->screen, nullptr);
        EXPECT_EQ(lmp->logfile, nullptr);
        EXPECT_GE(lmp->initclock, 0.0);

        EXPECT_EQ(lmp->suffix_enable, 1);
        EXPECT_STREQ(lmp->suffix, "kk");
        EXPECT_EQ(lmp->suffix2, nullptr);

        EXPECT_STREQ(lmp->exename, "LAMMPS_test");
        EXPECT_EQ(lmp->num_package, 0);
        EXPECT_EQ(lmp->clientserver, 0);

        EXPECT_NE(lmp->kokkos, nullptr);
        EXPECT_NE(lmp->atomKK, nullptr);
        EXPECT_NE(lmp->memoryKK, nullptr);
        EXPECT_NE(lmp->python, nullptr);
        EXPECT_NE(lmp->citeme, nullptr);
        if (LAMMPS::has_git_info) {
            EXPECT_STRNE(LAMMPS::git_commit,"");
            EXPECT_STRNE(LAMMPS::git_branch,"");
            EXPECT_STRNE(LAMMPS::git_descriptor,"");
        } else {
            EXPECT_STREQ(LAMMPS::git_commit,"(unknown)");
            EXPECT_STREQ(LAMMPS::git_branch,"(unknown)");
            EXPECT_STREQ(LAMMPS::git_descriptor,"(unknown)");
        }
    }

    // check help message printing
    TEST(LAMMPS_help, HelpMessage) {
        const char *args[] = {"LAMMPS_test", "-h"};
        char **argv = (char **)args;
        int argc = sizeof(args)/sizeof(char *);

        ::testing::internal::CaptureStdout();
        LAMMPS *lmp = new LAMMPS(argc, argv, MPI_COMM_WORLD);
        std::string output = testing::internal::GetCapturedStdout();
        EXPECT_STREQ(output.substr(0,61).c_str(),
                     "\nLarge-scale Atomic/Molecular Massively Parallel Simulator -");
        delete lmp;
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
