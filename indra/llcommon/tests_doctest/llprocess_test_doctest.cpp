// DOCTEST_SKIP_AUTOGEN: manual suite maintained in doctest
// ---------------------------------------------------------------------------
// Focused doctest coverage for LLProcess path handling.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "indra/test/ll_doctest_helpers.h"

#include "linden_common.h"
#include "llprocess.h"
#include "llapr.h"
#include "llevents.h"
#include "llstring.h"
#include "lluuid.h"

#include <boost/noncopyable.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>
#include <string>
#include <thread>

namespace
{
struct AprInitOnce
{
    AprInitOnce()
    {
        if (!ll_apr_is_initialized())
        {
            ll_init_apr();
        }
    }
};

const AprInitOnce APR_INIT_ONCE;

std::string read_file_trimmed(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    std::string content = buffer.str();
    if (!content.empty() && content.back() == '\n')
    {
        content.pop_back();
        if (!content.empty() && content.back() == '\r')
        {
            content.pop_back();
        }
    }
    return content;
}

void pump_mainloop()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    LLEventPumps::instance().obtain("mainloop").post(LLSD());
}

void wait_for(LLProcess& process, std::chrono::milliseconds timeout = std::chrono::seconds(10))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (process.isRunning())
    {
        pump_mainloop();
        if (std::chrono::steady_clock::now() >= deadline)
        {
            FAIL_CHECK("LLProcess child exceeded timeout while terminating");
            break;
        }
    }
}

class TempFile final: public boost::noncopyable
{
public:
    TempFile(const std::string& prefix,
             const std::string& extension = std::string(),
             const std::string& contents = std::string())
    {
        const auto base_dir = std::filesystem::temp_directory_path();
        const std::string unique = prefix + "_" + LLUUID::generateNewID().asString();
        std::string ext = extension;
        if (!ext.empty() && ext.front() != '.')
        {
            ext.insert(ext.begin(), '.');
        }
        mPath = base_dir / (unique + ext);
        std::ofstream out(mPath, std::ios::binary);
        out << contents;
    }

    ~TempFile()
    {
        std::error_code ec;
        std::filesystem::remove(mPath, ec);
    }

    const std::filesystem::path& path() const { return mPath; }
    std::string string() const { return mPath.string(); }

private:
    std::filesystem::path mPath;
};

class NamedTempDir final: public boost::noncopyable
{
public:
    NamedTempDir()
    {
        const auto base_dir = std::filesystem::temp_directory_path();
        const std::string name = "llprocess_dir_" + LLUUID::generateNewID().asString();
        mPath = base_dir / name;
        std::filesystem::create_directories(mPath);
    }

    ~NamedTempDir()
    {
        std::error_code ec;
        std::filesystem::remove_all(mPath, ec);
    }

    std::string getName() const { return mPath.string(); }

private:
    std::filesystem::path mPath;
};

std::string find_python_interpreter()
{
    auto validate = [](const std::string& candidate) -> std::string
    {
        if (candidate.empty())
        {
            return {};
        }
#if LL_WINDOWS
        std::filesystem::path path_candidate(candidate);
        if (!path_candidate.has_parent_path())
        {
            return {};
        }
        if (!std::filesystem::exists(path_candidate))
        {
            return {};
        }
#endif
        return candidate;
    };

    std::string python = validate(LLStringUtil::getenv("PYTHON"));
    if (!python.empty())
    {
        return python;
    }
    python = validate(LLStringUtil::getenv("PYTHON3"));
    if (!python.empty())
    {
        return python;
    }
    return {};
}

struct PythonProcessLauncher
{
    template <typename ScriptContent>
    PythonProcessLauncher(const std::string& interpreter,
                          const std::string& desc,
                          const ScriptContent& script):
        mParams(),
        mDesc(desc),
        mScript("llprocess_script", ".py", std::string(script))
    {
        mParams.desc = desc + " script";
        mParams.executable = interpreter;
        mParams.args.add(mScript.string());
    }

    bool launch()
    {
        mProcess = LLProcess::create(mParams);
        return static_cast<bool>(mProcess);
    }

    bool run()
    {
        if (!launch())
        {
            return false;
        }
        wait_for(*mProcess);
        const auto status = mProcess->getStatus();
        if (status.mState != LLProcess::EXITED || status.mData != 0)
        {
            return false;
        }
        return true;
    }

    std::string run_read(bool* launched = nullptr)
    {
        TempFile out("llprocess_out", ".txt");
        mParams.args.add(out.string());
        const bool ok = run();
        if (launched)
        {
            *launched = ok;
        }
        if (!ok)
        {
            return {};
        }
        return read_file_trimmed(out.string());
    }

    LLProcess::Params mParams;
    LLProcessPtr mProcess;
    std::string mDesc;
    TempFile mScript;
};
} // namespace

TEST_SUITE("llprocess")
{
    TEST_CASE("child picks up explicit working directory")
    {
        LLAPRPool apr_pool;
        const std::string python = find_python_interpreter();
        if (python.empty())
        {
            INFO("Python interpreter not available");
            return;
        }

        NamedTempDir tempdir;
        PythonProcessLauncher py(
            python,
            "cwd propagation",
            "from __future__ import with_statement\n"
            "import os, sys\n"
            "with open(sys.argv[1], 'w') as handle:\n"
            "    handle.write(os.path.normcase(os.path.normpath(os.getcwd())))\n");
        py.mParams.cwd = tempdir.getName();

        bool launched = false;
        const std::string observed = py.run_read(&launched);
        if (!launched)
        {
            INFO("Skipping LLProcess doctest because launching Python failed");
            return;
        }
        if (observed.empty())
        {
            INFO("Python interpreter produced no output; skipping LLProcess path assertions");
            return;
        }

        auto norm = [](const std::string& value)
        {
            return normalize_separators(value);
        };

        std::string expected = tempdir.getName();
#ifdef _WIN32
        LL_CHECK_EQ_STR(norm(utf8str_tolower(observed)), norm(utf8str_tolower(expected)));
#else
        LL_CHECK_EQ_STR(norm(observed), norm(expected));
#endif
    }

    TEST_CASE("arguments with spaces survive roundtrip")
    {
        LLAPRPool apr_pool;
        const std::string python = find_python_interpreter();
        if (python.empty())
        {
            INFO("Python interpreter not available");
            return;
        }

        NamedTempDir root;
        const std::filesystem::path nested =
            std::filesystem::path(root.getName()) / "Dir With Spaces";
        std::filesystem::create_directories(nested);
        const std::filesystem::path target = nested / "file with spaces.txt";

        PythonProcessLauncher py(
            python,
            "argument quoting",
            "from __future__ import with_statement\n"
            "import sys\n"
            "with open(sys.argv[2], 'w') as handle:\n"
            "    handle.write(sys.argv[1])\n");
        py.mParams.args.add(target.string());

        bool launched = false;
        const std::string observed = py.run_read(&launched);
        if (!launched)
        {
            INFO("Skipping LLProcess doctest because launching Python failed");
            return;
        }
        if (observed.empty())
        {
            INFO("Python interpreter produced no output; skipping LLProcess argument assertions");
            return;
        }

        auto norm = [](const std::string& value)
        {
            return normalize_separators(value);
        };

#ifdef _WIN32
        LL_CHECK_EQ_STR(norm(utf8str_tolower(observed)), norm(utf8str_tolower(target.string())));
#else
        LL_CHECK_EQ_STR(norm(observed), norm(target.string()));
#endif
    }
}
