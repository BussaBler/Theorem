import multiprocessing
import os
import platform
import shutil
import subprocess
import sys

from SCons.Script import (
    ARGUMENTS,
    Alias,
    Default,
    Depends,
    Dir,
    Environment,
    Exit,
    GetOption,
    Help,
    SConscript,
    SetOption,
)


def getPlatform():
    osName = platform.system().lower()
    if osName.startswith("win"):
        return "windows"
    elif osName.startswith("linux"):
        return "linux"
    elif osName.startswith("darwin"):
        return "macos"
    return osName


COLORS = {
    "reset": "\033[0m",
    "bold": "\033[1m",
    "green": "\033[32m",
    "blue": "\033[34m",
    "yellow": "\033[33m",
    "cyan": "\033[36m",
    "magenta": "\033[35m",
    "red": "\033[31m",
}


def printWithColor(*objects, color, sep=" ", end="\n", file=None, flush=False):
    print(f"{color}", end="", file=file, flush=flush)
    print(*objects, "\033[0m", sep=sep, end=end, file=file, flush=flush)


SetOption("num_jobs", multiprocessing.cpu_count())
current_platform = getPlatform()
architecture = platform.machine()
if architecture == "AMD64":
    architecture = "x86_64"
CPPVER = "23"

DEFAULT_COMPILER = (
    "msvc"
    if current_platform == "windows"
    else ("clang" if current_platform == "macos" else "gcc")
)
DEFAULT_RENDERER = "metal" if current_platform == "macos" else "vulkan"

ALLOWED_CONFIGS = ["debug", "release"]
ALLOWED_PLATFORMS = ["windows", "linux", "macos"]
ALLOWED_RENDERERS = ["vulkan", "metal"]
ALLOWED_COMPILERS = ["msvc", "gcc", "g++", "clang"]


def get_enum_arg(name, default, allowed):
    val = ARGUMENTS.get(name, default).lower()
    if val not in allowed:
        printWithColor(
            f"ERROR: Invalid '{name}' = '{val}'. Allowed values are: {', '.join(allowed)}",
            color=COLORS["red"],
        )
        Exit(1)
    return val


def get_bool_arg(name, default):
    raw_val = ARGUMENTS.get(name, str(default)).lower()
    if raw_val in ["yes", "true", "1"]:
        return True
    if raw_val in ["no", "false", "0"]:
        return False
    printWithColor(
        f"ERROR: Invalid boolean value for '{name}' = '{raw_val}'. Use yes/no, true/false, or 1/0.",
        color=COLORS["red"],
    )
    Exit(1)


targetPlatform = get_enum_arg("platform", current_platform, ALLOWED_PLATFORMS)
buildConfig = get_enum_arg("config", "debug", ALLOWED_CONFIGS)
renderBackend = get_enum_arg("renderer", DEFAULT_RENDERER, ALLOWED_RENDERERS)
compilerType = get_enum_arg("compiler", DEFAULT_COMPILER, ALLOWED_COMPILERS)

vsproj = get_bool_arg("vsproj", False)
verbose = get_bool_arg("verbose", False)
disableColors = get_bool_arg("no-color", False)

Help(f"""
ProjectAxiom Build System

Available Configurations:
  config=<debug|release>       (default: debug)
  platform=<windows|linux|macos> (default: auto-detected [{current_platform}])
  compiler=<msvc|gcc|clang>    (default: OS specific [{DEFAULT_COMPILER}])
  renderer=<vulkan|metal>      (default: OS specific [{DEFAULT_RENDERER}])

Flags:
  vsproj=<yes|no>              Generate Visual Studio solution (default: no)
  verbose=<yes|no>             Print full compiler commands (default: no)
  no-color=<yes|no>            Disable colored build output (default: no)

Standard SCons flags:
  -c, --clean                  Clean the build outputs
  -j N                         Run N jobs in parallel
""")


def detectMsvc():
    env = Environment(tools=["default", "msvc"])
    return bool(env.WhereIs("cl") or env.WhereIs("cl.exe"))


def detectCompilerTools():
    if targetPlatform == "windows":
        match compilerType:
            case "msvc":
                if detectMsvc():
                    return ["msvc", "mslib", "mslink"]
                printWithColor("ERROR: No MSVC compiler found.", color=COLORS["red"])
                Exit(1)
            case "gcc" | "g++":
                if shutil.which("g++") or shutil.which("gcc"):
                    return ["gcc", "g++", "ar", "link"]
                printWithColor("ERROR: No GCC compiler found.", color=COLORS["red"])
                Exit(1)
            case "clang":
                if shutil.which("clang") or shutil.which("clang++"):
                    return ["clang", "clang++", "llvm-ar", "llvm-link"]
                printWithColor("ERROR: No Clang compiler found.", color=COLORS["red"])
                Exit(1)
            case _:
                printWithColor(
                    f"ERROR: Compiler '{compilerType}' is not supported on Windows.",
                    color=COLORS["red"],
                )
                Exit(1)

    elif targetPlatform == "linux":
        match compilerType:
            case "gcc" | "g++":
                if shutil.which("g++") or shutil.which("gcc"):
                    return ["gcc", "g++", "ar", "link"]
                printWithColor("ERROR: No GCC compiler found.", color=COLORS["red"])
                Exit(1)
            case "clang":
                if shutil.which("clang") or shutil.which("clang++"):
                    return ["clang", "clang++", "llvm-ar", "llvm-link"]
                printWithColor("ERROR: No Clang compiler found.", color=COLORS["red"])
                Exit(1)
            case _:
                printWithColor(
                    f"ERROR: Compiler '{compilerType}' is not supported on Linux.",
                    color=COLORS["red"],
                )
                Exit(1)

    elif targetPlatform == "macos":
        match compilerType:
            case "gcc" | "g++":
                if shutil.which("g++") or shutil.which("gcc"):
                    return ["gcc", "g++", "ar", "link"]
                printWithColor("ERROR: No GCC compiler found.", color=COLORS["red"])
                Exit(1)
            case "clang":
                if shutil.which("clang") or shutil.which("clang++"):
                    return ["clang", "clang++", "ar", "applelink"]
                printWithColor("ERROR: No Clang compiler found.", color=COLORS["red"])
                Exit(1)
            case _:
                printWithColor(
                    f"ERROR: Compiler '{compilerType}' is not supported on macOS.",
                    color=COLORS["red"],
                )
                Exit(1)


tools = detectCompilerTools()
tools.append("compilation_db")

printWithColor(
    f"Platform: {targetPlatform}, Compiler: {compilerType}, Architecture: {architecture}",
    color=COLORS["cyan"],
)

if vsproj:
    if compilerType == "msvc":
        tools.extend(["msvs"])
        printWithColor("Visual Studio project generation enabled", color=COLORS["cyan"])
    else:
        printWithColor(
            "ERROR: Visual Studio project generation requires the MSVC compiler.",
            color=COLORS["red"],
        )
        Exit(1)

baseEnv = Environment(tools=tools, ENV=os.environ)


def buildCMakeLibs(env):
    vendorDir = Dir("Vendor").get_path()
    shadercDir = os.path.join(vendorDir, "Shaderc")
    shadercBuildDir = os.path.join(shadercDir, f"Build/{compilerType}")

    if compilerType == "msvc":
        expectedLib = os.path.join(
            shadercBuildDir,
            "libshaderc",
            buildConfig.capitalize(),
            "shaderc_combined.lib",
        )
    else:
        expectedLib = os.path.join(
            shadercBuildDir, "libshaderc", "libshaderc_combined.a"
        )

    if os.path.exists(expectedLib):
        printWithColor(
            f"Found existing Shaderc library at {expectedLib}, skipping build.",
            color=COLORS["yellow"],
        )
        return expectedLib

    spirvToolsDir = os.path.join(
        shadercDir, "third_party", "spirv-tools", "CMakeLists.txt"
    )
    if not os.path.exists(spirvToolsDir):
        syncScript = os.path.join(shadercDir, "utils", "git-sync-deps")
        try:
            subprocess.run([sys.executable, syncScript], check=True)
        except subprocess.CalledProcessError:
            printWithColor(
                "ERROR: Failed to sync Shaderc dependencies. Ensure you have Python/Git installed.",
                color=COLORS["red"],
            )
            Exit(1)

    cmakeConfigCmd = [
        "cmake",
        "-S",
        shadercDir,
        "-B",
        shadercBuildDir,
        "-DSHADERC_SKIP_TESTS=ON",
        "-DSHADERC_SKIP_EXAMPLES=ON",
        "-DSHADERC_SKIP_COPYRIGHT_CHECK=ON",
        "-DSHADERC_ENABLE_SHARED_CRT=ON",
        f"-DCMAKE_BUILD_TYPE={buildConfig.capitalize()}",
    ]

    if compilerType == "msvc":
        cmakeConfigCmd.extend(["-DCMAKE_C_COMPILER=cl", "-DCMAKE_CXX_COMPILER=cl"])
    elif compilerType in ["gcc", "g++"]:
        cmakeConfigCmd.extend(["-DCMAKE_C_COMPILER=gcc", "-DCMAKE_CXX_COMPILER=g++"])
        if targetPlatform == "windows":
            cmakeConfigCmd.extend(["-G", "MinGW Makefiles"])
    elif compilerType == "clang":
        cmakeConfigCmd.extend(
            ["-DCMAKE_C_COMPILER=clang", "-DCMAKE_CXX_COMPILER=clang++"]
        )

    cmakeBuildCmd = [
        "cmake",
        "--build",
        shadercBuildDir,
        "--config",
        buildConfig.capitalize(),
        "--parallel",
        str(multiprocessing.cpu_count()),
    ]

    printWithColor("[VENDOR]", color=COLORS["green"], end="")
    printWithColor(
        "Building Shaderc library (this might take a while)", color=COLORS["reset"]
    )
    try:
        sconsEnvVars = env["ENV"]
        subprocess.run(
            cmakeConfigCmd, check=True, stdout=subprocess.DEVNULL, env=sconsEnvVars
        )
        subprocess.run(
            cmakeBuildCmd, check=True, stdout=subprocess.DEVNULL, env=sconsEnvVars
        )
    except subprocess.CalledProcessError as e:
        printWithColor(
            f"ERROR: Failed to build Shaderc library. Command '{' '.join(e.cmd)}' exited with code {e.returncode}.",
            color=COLORS["red"],
        )
        Exit(1)

    return expectedLib


shadercLibPath = buildCMakeLibs(baseEnv)

baseEnv.Append(
    CPPPATH=[Dir("Vendor/Shaderc/libshaderc/include")],
    LIBPATH=[Dir(os.path.dirname(shadercLibPath))],
    LIBS=["shaderc_combined"],
)

if renderBackend == "vulkan":
    baseEnv.Append(CPPPATH=[Dir("Vendor/Vulkan/Include")])
elif renderBackend == "metal":
    baseEnv.Append(CPPPATH=[Dir("Vendor")])

if targetPlatform == "windows":
    baseEnv.Append(LIBS=["user32", "gdi32", "winmm"])
elif targetPlatform == "linux":
    baseEnv.Append(LIBS=["X11"])
elif targetPlatform == "macos":
    baseEnv.Append(FRAMEWORKS=["Cocoa", "IOKit", "CoreVideo", "Metal", "QuartzCore"])


def getBuildFlags(compiler):
    if compiler == "msvc":
        return {
            "debugCcflags": [
                "/Zi",
                "/Od",
                "/EHsc",
                "/nologo",
                "/FS",
                "/MDd",
                "/permissive-",
            ],
            "releaseCcflags": ["/O2", "/EHsc", "/nologo", "/FS", "/MD"],
            "debugLinkflags": ["/DEBUG", "/nologo"],
            "releaseLinkflags": ["/nologo"],
            "debugDefines": ["AX_DEBUG", "AX_ENABLE_ASSERTS"],
            "releaseDefines": ["AX_RELEASE"],
            "version": ["/std:c++" + CPPVER + "preview"],
        }
    elif compiler in ["gcc", "g++"]:
        return {
            "debugCcflags": ["-g", "-O0", "-pedantic"],
            "releaseCcflags": ["-O3", "-march=native"],
            "debugLinkflags": [],
            "releaseLinkflags": [],
            "debugDefines": ["AX_DEBUG", "AX_ENABLE_ASSERTS"],
            "releaseDefines": ["AX_RELEASE"],
            "version": ["-std=c++" + CPPVER],
        }
    else:
        return {
            "debugCcflags": ["-g", "-O0", "-pedantic", "-fexperimental-library"],
            "releaseCcflags": ["-O3", "-march=native", "-fexperimental-library"],
            "debugLinkflags": ["-fexperimental-library"],
            "releaseLinkflags": ["-fexperimental-library"],
            "debugDefines": ["AX_DEBUG", "AX_ENABLE_ASSERTS"],
            "releaseDefines": ["AX_RELEASE"],
            "version": ["-std=c++" + CPPVER],
        }


flags = getBuildFlags(compilerType)

debugEnv = baseEnv.Clone(
    CCFLAGS=flags["debugCcflags"],
    LINKFLAGS=flags["debugLinkflags"],
    CPPDEFINES=flags["debugDefines"],
    CXXFLAGS=flags["version"],
)

releaseEnv = baseEnv.Clone(
    CCFLAGS=flags["releaseCcflags"],
    LINKFLAGS=flags["releaseLinkflags"],
    CPPDEFINES=flags["releaseDefines"],
    CXXFLAGS=flags["version"],
)

buildInfo = {
    "platform": targetPlatform,
    "architecture": architecture,
    "compiler": compilerType,
    "config": buildConfig,
    "vsproj": vsproj,
    "renderer": renderBackend,
}


def setupOutputColors(env_target):
    if verbose:
        return

    if disableColors:
        env_target["CXXCOMSTR"] = "[COMPILE] $SOURCE"
        env_target["CCCOMSTR"] = "[COMPILE] $SOURCE"
        env_target["LINKCOMSTR"] = "[LINK] $TARGET"
        env_target["ARCOMSTR"] = "[ARCHIVE] $TARGET"
        env_target["RANLIBCOMSTR"] = "[RANLIB] $TARGET"
        env_target["LIBCOMSTR"] = "[LIB] $TARGET"
    else:
        env_target["CXXCOMSTR"] = f"{COLORS['green']}[COMPILE]{COLORS['reset']} $SOURCE"
        env_target["CCCOMSTR"] = f"{COLORS['green']}[COMPILE]{COLORS['reset']} $SOURCE"
        env_target["LINKCOMSTR"] = f"{COLORS['yellow']}[LINK]{COLORS['reset']} $TARGET"
        env_target["ARCOMSTR"] = f"{COLORS['blue']}[ARCHIVE]{COLORS['reset']} $TARGET"
        env_target["RANLIBCOMSTR"] = (
            f"{COLORS['blue']}[RANLIB]{COLORS['reset']} $TARGET"
        )
        env_target["LIBCOMSTR"] = f"{COLORS['blue']}[LIB]{COLORS['reset']} $TARGET"


def printInfo():
    action = "Cleaning" if GetOption("clean") else "Building"
    printWithColor(
        f"\n{action} {buildConfig.capitalize()} configuration for {targetPlatform}-{architecture}",
        color=COLORS["cyan"],
    )
    printWithColor(f" Compiler: {compilerType}", color=COLORS["magenta"])
    printWithColor(f" Renderer: {renderBackend}", color=COLORS["magenta"])
    if not verbose:
        printWithColor(
            " (Use 'verbose=yes' to see full command lines)", color=COLORS["yellow"]
        )
    print("")


printInfo()
setupOutputColors(baseEnv)
setupOutputColors(debugEnv)
setupOutputColors(releaseEnv)

env = debugEnv if buildConfig == "debug" else releaseEnv

axImageLoaderLib = SConscript(
    "Vendor/AxImageLoader/SConscript",
    variant_dir=f"Bin-Int/{buildConfig.capitalize()}/AxImageLoader",
    src_dir="Vendor/AxImageLoader",
    duplicate=0,
    exports=["env", "buildInfo"],
)

spirvCrossLib = SConscript(
    "Vendor/SpirvCross/SConscript",
    variant_dir=f"Bin-Int/{buildConfig.capitalize()}/SpirvCross",
    src_dir="Vendor/SpirvCross",
    duplicate=0,
    exports=["env", "buildInfo"],
)

axModelLoaderLib = SConscript(
    "Vendor/AxModelLoader/SConscript",
    variant_dir=f"Bin-Int/{buildConfig.capitalize()}/AxModelLoader",
    src_dir="Vendor/AxModelLoader",
    duplicate=0,
    exports=["env", "buildInfo"],
)

axiomLib, axiomProject = SConscript(
    "Axiom/SConscript",
    variant_dir=f"Bin-Int/{buildConfig.capitalize()}/Axiom",
    src_dir="Axiom",
    duplicate=0,
    exports=[
        "baseEnv",
        "debugEnv",
        "releaseEnv",
        "buildInfo",
        "axImageLoaderLib",
        "spirvCrossLib",
        "axModelLoaderLib",
    ],
)

theoremApp, theoremProject = SConscript(
    "Theorem/SConscript",
    variant_dir=f"Bin-Int/{buildConfig.capitalize()}/Theorem",
    src_dir="Theorem",
    duplicate=0,
    exports=[
        "baseEnv",
        "debugEnv",
        "releaseEnv",
        "buildInfo",
        "axiomLib",
        "axImageLoaderLib",
        "spirvCrossLib",
        "axModelLoaderLib",
    ],
)

Depends(theoremProject, axiomProject)
Default(theoremApp)

cdb = baseEnv.CompilationDatabase("compile_commands.json")
Alias("cdb", cdb)

if vsproj and compilerType == "msvc":
    projectAxiomSolution = baseEnv.MSVSSolution(
        target="ProjectAxiom" + baseEnv["MSVSSOLUTIONSUFFIX"],
        projects=[theoremProject, axiomProject],
        variant=["Debug|x64"],
    )
    Alias("ProjectAxiom", projectAxiomSolution)
    Default("ProjectAxiom")

    if GetOption("clean"):
        action, color, symbol = "cleaned", COLORS["red"], "-"
    else:
        action, color, symbol = "generated", COLORS["green"], "+"

    printWithColor(
        f"Visual Studio projects and solution will be {action}:", color=COLORS["cyan"]
    )
    printWithColor(f" {symbol}", "Axiom.vcxproj", color=color)
    printWithColor(f" {symbol}", "Theorem.vcxproj", color=color)
    printWithColor(f" {symbol}", "AxiomEngine.sln", color=color)
