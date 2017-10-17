

#include "Collation.h"

#include "frameworks/base/cmds/statsd/src/stats_events.pb.h"

#include <set>
#include <vector>

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace google::protobuf;
using namespace std;

namespace android {
namespace stats_log_api_gen {

using android::os::statsd::StatsEvent;

// TODO: Support WorkSources

/**
 * Turn lower and camel case into upper case with underscores.
 */
static string
make_constant_name(const string& str)
{
    string result;
    const int N = str.size();
    bool underscore_next = false;
    for (int i=0; i<N; i++) {
        char c = str[i];
        if (c >= 'A' && c <= 'Z') {
            if (underscore_next) {
                result += '_';
                underscore_next = false;
            }
        } else if (c >= 'a' && c <= 'z') {
            c = 'A' + c - 'a';
            underscore_next = true;
        } else if (c == '_') {
            underscore_next = false;
        }
        result += c;
    }
    return result;
}

static const char*
cpp_type_name(java_type_t type)
{
    switch (type) {
        case JAVA_TYPE_BOOLEAN:
            return "bool";
        case JAVA_TYPE_INT:
            return "int32_t";
        case JAVA_TYPE_LONG:
            return "int64_t";
        case JAVA_TYPE_FLOAT:
            return "float";
        case JAVA_TYPE_DOUBLE:
            return "double";
        case JAVA_TYPE_STRING:
            return "char const*";
        default:
            return "UNKNOWN";
    }
}

static const char*
java_type_name(java_type_t type)
{
    switch (type) {
        case JAVA_TYPE_BOOLEAN:
            return "boolean";
        case JAVA_TYPE_INT:
            return "int";
        case JAVA_TYPE_LONG:
            return "long";
        case JAVA_TYPE_FLOAT:
            return "float";
        case JAVA_TYPE_DOUBLE:
            return "double";
        case JAVA_TYPE_STRING:
            return "java.lang.String";
        default:
            return "UNKNOWN";
    }
}

static int
write_stats_log_cpp(FILE* out, const Atoms& atoms)
{
    int errorCount;

    // Print prelude
    fprintf(out, "// This file is autogenerated\n");
    fprintf(out, "\n");

    fprintf(out, "#include <log/log_event_list.h>\n");
    fprintf(out, "#include <log/log.h>\n");
    fprintf(out, "#include <statslog.h>\n");
    fprintf(out, "\n");

    fprintf(out, "namespace android {\n");
    fprintf(out, "namespace util {\n");

    // Print write methods
    fprintf(out, "\n");
    for (set<vector<java_type_t>>::const_iterator signature = atoms.signatures.begin();
            signature != atoms.signatures.end(); signature++) {
        int argIndex;

        fprintf(out, "void\n");
        fprintf(out, "stats_write(int code");
        argIndex = 1;
        for (vector<java_type_t>::const_iterator arg = signature->begin();
                arg != signature->end(); arg++) {
            fprintf(out, ", %s arg%d", cpp_type_name(*arg), argIndex);
            argIndex++;
        }
        fprintf(out, ")\n");

        fprintf(out, "{\n");
        argIndex = 1;
        fprintf(out, "    android_log_event_list event(code);\n");
        for (vector<java_type_t>::const_iterator arg = signature->begin();
                arg != signature->end(); arg++) {
            if (*arg == JAVA_TYPE_STRING) {
                fprintf(out, "    if (arg%d == NULL) {\n", argIndex);
                fprintf(out, "        arg%d = \"\";\n", argIndex);
                fprintf(out, "    }\n");
            }
            fprintf(out, "    event << arg%d;\n", argIndex);
            argIndex++;
        }

        fprintf(out, "    event.write(LOG_ID_STATS);\n");
        fprintf(out, "}\n");
        fprintf(out, "\n");
    }

    // Print footer
    fprintf(out, "\n");
    fprintf(out, "} // namespace util\n");
    fprintf(out, "} // namespace android\n");

    return 0;
}


static int
write_stats_log_header(FILE* out, const Atoms& atoms)
{
    int errorCount;

    // Print prelude
    fprintf(out, "// This file is autogenerated\n");
    fprintf(out, "\n");
    fprintf(out, "#pragma once\n");
    fprintf(out, "\n");
    fprintf(out, "#include <stdint.h>\n");
    fprintf(out, "\n");

    fprintf(out, "namespace android {\n");
    fprintf(out, "namespace util {\n");
    fprintf(out, "\n");
    fprintf(out, "/*\n");
    fprintf(out, " * API For logging statistics events.\n");
    fprintf(out, " */\n");
    fprintf(out, "\n");
    fprintf(out, "/**\n");
    fprintf(out, " * Constants for event codes.\n");
    fprintf(out, " */\n");
    fprintf(out, "enum {\n");

    size_t i = 0;
    // Print constants
    for (set<AtomDecl>::const_iterator atom = atoms.decls.begin();
            atom != atoms.decls.end(); atom++) {
        string constant = make_constant_name(atom->name);
        fprintf(out, "\n");
        fprintf(out, "    /**\n");
        fprintf(out, "     * %s %s\n", atom->message.c_str(), atom->name.c_str());
        fprintf(out, "     * Usage: stats_write(StatsLog.%s", constant.c_str());
        for (vector<AtomField>::const_iterator field = atom->fields.begin();
                field != atom->fields.end(); field++) {
            fprintf(out, ", %s %s", cpp_type_name(field->javaType), field->name.c_str());
        }
        fprintf(out, ");\n");
        fprintf(out, "     */\n");
        char const* const comma = (i == atoms.decls.size() - 1) ? "" : ",";
        fprintf(out, "    %s = %d%s\n", constant.c_str(), atom->code, comma);
        i++;
    }
    fprintf(out, "\n");
    fprintf(out, "};\n");
    fprintf(out, "\n");

    // Print write methods
    fprintf(out, "//\n");
    fprintf(out, "// Write methods\n");
    fprintf(out, "//\n");
    for (set<vector<java_type_t>>::const_iterator signature = atoms.signatures.begin();
            signature != atoms.signatures.end(); signature++) {

        fprintf(out, "void stats_write(int code");
        int argIndex = 1;
        for (vector<java_type_t>::const_iterator arg = signature->begin();
                arg != signature->end(); arg++) {
            fprintf(out, ", %s arg%d", cpp_type_name(*arg), argIndex);
            argIndex++;
        }
        fprintf(out, ");\n");
    }

    fprintf(out, "\n");
    fprintf(out, "} // namespace util\n");
    fprintf(out, "} // namespace android\n");

    return 0;
}

static int
write_stats_log_java(FILE* out, const Atoms& atoms)
{
    int errorCount;

    // Print prelude
    fprintf(out, "// This file is autogenerated\n");
    fprintf(out, "\n");
    fprintf(out, "package android.util;\n");
    fprintf(out, "\n");
    fprintf(out, "\n");
    fprintf(out, "/**\n");
    fprintf(out, " * API For logging statistics events.\n");
    fprintf(out, " * @hide\n");
    fprintf(out, " */\n");
    fprintf(out, "public final class StatsLog {\n");
    fprintf(out, "    // Constants for event codes.\n");

    // Print constants
    for (set<AtomDecl>::const_iterator atom = atoms.decls.begin();
            atom != atoms.decls.end(); atom++) {
        string constant = make_constant_name(atom->name);
        fprintf(out, "\n");
        fprintf(out, "    /**\n");
        fprintf(out, "     * %s %s\n", atom->message.c_str(), atom->name.c_str());
        fprintf(out, "     * Usage: StatsLog.write(StatsLog.%s", constant.c_str());
        for (vector<AtomField>::const_iterator field = atom->fields.begin();
                field != atom->fields.end(); field++) {
            fprintf(out, ", %s %s", java_type_name(field->javaType), field->name.c_str());
        }
        fprintf(out, ");\n");
        fprintf(out, "     */\n");
        fprintf(out, "    public static final int %s = %d;\n", constant.c_str(), atom->code);
    }
    fprintf(out, "\n");

    // Print write methods
    fprintf(out, "    // Write methods\n");
    for (set<vector<java_type_t>>::const_iterator signature = atoms.signatures.begin();
            signature != atoms.signatures.end(); signature++) {
        fprintf(out, "    public static native void write(int code");
        int argIndex = 1;
        for (vector<java_type_t>::const_iterator arg = signature->begin();
                arg != signature->end(); arg++) {
            fprintf(out, ", %s arg%d", java_type_name(*arg), argIndex);
            argIndex++;
        }
        fprintf(out, ");\n");
    }

    fprintf(out, "}\n");

    return 0;
}

static const char*
jni_type_name(java_type_t type)
{
    switch (type) {
        case JAVA_TYPE_BOOLEAN:
            return "jboolean";
        case JAVA_TYPE_INT:
            return "jint";
        case JAVA_TYPE_LONG:
            return "jlong";
        case JAVA_TYPE_FLOAT:
            return "jfloat";
        case JAVA_TYPE_DOUBLE:
            return "jdouble";
        case JAVA_TYPE_STRING:
            return "jstring";
        default:
            return "UNKNOWN";
    }
}

static string
jni_function_name(const vector<java_type_t>& signature)
{
    string result("StatsLog_write");
    for (vector<java_type_t>::const_iterator arg = signature.begin();
            arg != signature.end(); arg++) {
        switch (*arg) {
            case JAVA_TYPE_BOOLEAN:
                result += "_boolean";
                break;
            case JAVA_TYPE_INT:
                result += "_int";
                break;
            case JAVA_TYPE_LONG:
                result += "_long";
                break;
            case JAVA_TYPE_FLOAT:
                result += "_float";
                break;
            case JAVA_TYPE_DOUBLE:
                result += "_double";
                break;
            case JAVA_TYPE_STRING:
                result += "_String";
                break;
            default:
                result += "_UNKNOWN";
                break;
        }
    }
    return result;
}

static const char*
java_type_signature(java_type_t type)
{
    switch (type) {
        case JAVA_TYPE_BOOLEAN:
            return "Z";
        case JAVA_TYPE_INT:
            return "I";
        case JAVA_TYPE_LONG:
            return "J";
        case JAVA_TYPE_FLOAT:
            return "F";
        case JAVA_TYPE_DOUBLE:
            return "D";
        case JAVA_TYPE_STRING:
            return "Ljava/lang/String;";
        default:
            return "UNKNOWN";
    }
}

static string
jni_function_signature(const vector<java_type_t>& signature)
{
    string result("(I");
    for (vector<java_type_t>::const_iterator arg = signature.begin();
            arg != signature.end(); arg++) {
        result += java_type_signature(*arg);
    }
    result += ")V";
    return result;
}

static int
write_stats_log_jni(FILE* out, const Atoms& atoms)
{
    int errorCount;

    // Print prelude
    fprintf(out, "// This file is autogenerated\n");
    fprintf(out, "\n");

    fprintf(out, "#include <statslog.h>\n");
    fprintf(out, "\n");
    fprintf(out, "#include <nativehelper/JNIHelp.h>\n");
    fprintf(out, "#include \"core_jni_helpers.h\"\n");
    fprintf(out, "#include \"jni.h\"\n");
    fprintf(out, "\n");
    fprintf(out, "#define UNUSED  __attribute__((__unused__))\n");
    fprintf(out, "\n");

    fprintf(out, "namespace android {\n");
    fprintf(out, "\n");

    // Print write methods
    for (set<vector<java_type_t>>::const_iterator signature = atoms.signatures.begin();
            signature != atoms.signatures.end(); signature++) {
        int argIndex;

        fprintf(out, "static void\n");
        fprintf(out, "%s(JNIEnv* env, jobject clazz UNUSED, jint code",
                jni_function_name(*signature).c_str());
        argIndex = 1;
        for (vector<java_type_t>::const_iterator arg = signature->begin();
                arg != signature->end(); arg++) {
            fprintf(out, ", %s arg%d", jni_type_name(*arg), argIndex);
            argIndex++;
        }
        fprintf(out, ")\n");

        fprintf(out, "{\n");

        // Prepare strings
        argIndex = 1;
        bool hadString = false;
        for (vector<java_type_t>::const_iterator arg = signature->begin();
                arg != signature->end(); arg++) {
            if (*arg == JAVA_TYPE_STRING) {
                fprintf(out, "    const char* str%d;\n", argIndex);
                fprintf(out, "    if (arg%d != NULL) {\n", argIndex);
                fprintf(out, "        str%d = env->GetStringUTFChars(arg%d, NULL);\n",
                        argIndex, argIndex);
                fprintf(out, "    } else {\n");
                fprintf(out, "        str%d = NULL;\n", argIndex);
                fprintf(out, "    }\n");
                hadString = true;
            }
            argIndex++;
        }

        // Emit this to quiet the unused parameter warning if there were no strings.
        if (!hadString) {
            fprintf(out, "    (void)env;\n");
        }

        // stats_write call
        argIndex = 1;
        fprintf(out, "    android::util::stats_write(code");
        for (vector<java_type_t>::const_iterator arg = signature->begin();
                arg != signature->end(); arg++) {
            const char* argName = (*arg == JAVA_TYPE_STRING) ? "str" : "arg";
            fprintf(out, ", (%s)%s%d", cpp_type_name(*arg), argName, argIndex);
            argIndex++;
        }
        fprintf(out, ");\n");

        // Clean up strings
        argIndex = 1;
        for (vector<java_type_t>::const_iterator arg = signature->begin();
                arg != signature->end(); arg++) {
            if (*arg == JAVA_TYPE_STRING) {
                fprintf(out, "    if (str%d != NULL) {\n", argIndex);
                fprintf(out, "        env->ReleaseStringUTFChars(arg%d, str%d);\n",
                        argIndex, argIndex);
                fprintf(out, "    }\n");
            }
            argIndex++;
        }

        fprintf(out, "}\n");
        fprintf(out, "\n");
    }

    // Print registration function table
    fprintf(out, "/*\n");
    fprintf(out, " * JNI registration.\n");
    fprintf(out, " */\n");
    fprintf(out, "static const JNINativeMethod gRegisterMethods[] = {\n");
    for (set<vector<java_type_t>>::const_iterator signature = atoms.signatures.begin();
            signature != atoms.signatures.end(); signature++) {
        fprintf(out, "    { \"write\", \"%s\", (void*)%s },\n",
                jni_function_signature(*signature).c_str(),
                jni_function_name(*signature).c_str());
    }
    fprintf(out, "};\n");
    fprintf(out, "\n");

    // Print registration function
    fprintf(out, "int register_android_util_StatsLog(JNIEnv* env) {\n");
    fprintf(out, "    return RegisterMethodsOrDie(\n");
    fprintf(out, "            env,\n");
    fprintf(out, "            \"android/util/StatsLog\",\n");
    fprintf(out, "            gRegisterMethods, NELEM(gRegisterMethods));\n");
    fprintf(out, "}\n");

    fprintf(out, "\n");
    fprintf(out, "} // namespace android\n");

    return 0;
}


static void
print_usage()
{
    fprintf(stderr, "usage: stats-log-api-gen OPTIONS\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "OPTIONS\n");
    fprintf(stderr, "  --cpp FILENAME       the header file to output\n");
    fprintf(stderr, "  --header FILENAME    the cpp file to output\n");
    fprintf(stderr, "  --help               this message\n");
    fprintf(stderr, "  --java FILENAME      the java file to output\n");
    fprintf(stderr, "  --jni FILENAME       the jni file to output\n");
}

/**
 * Do the argument parsing and execute the tasks.
 */
static int
run(int argc, char const*const* argv)
{
    string cppFilename;
    string headerFilename;
    string javaFilename;
    string jniFilename;

    int index = 1;
    while (index < argc) {
        if (0 == strcmp("--help", argv[index])) {
            print_usage();
            return 0;
        } else if (0 == strcmp("--cpp", argv[index])) {
            index++;
            if (index >= argc) {
                print_usage();
                return 1;
            }
            cppFilename = argv[index];
        } else if (0 == strcmp("--header", argv[index])) {
            index++;
            if (index >= argc) {
                print_usage();
                return 1;
            }
            headerFilename = argv[index];
        } else if (0 == strcmp("--java", argv[index])) {
            index++;
            if (index >= argc) {
                print_usage();
                return 1;
            }
            javaFilename = argv[index];
        } else if (0 == strcmp("--jni", argv[index])) {
            index++;
            if (index >= argc) {
                print_usage();
                return 1;
            }
            jniFilename = argv[index];
        }
        index++;
    }

    if (cppFilename.size() == 0
            && headerFilename.size() == 0
            && javaFilename.size() == 0
            && jniFilename.size() == 0) {
        print_usage();
        return 1;
    }

    // Collate the parameters
    Atoms atoms;
    int errorCount = collate_atoms(StatsEvent::descriptor(), &atoms);
    if (errorCount != 0) {
        return 1;
    }

    // Write the .cpp file
    if (cppFilename.size() != 0) {
        FILE* out = fopen(cppFilename.c_str(), "w");
        if (out == NULL) {
            fprintf(stderr, "Unable to open file for write: %s\n", cppFilename.c_str());
            return 1;
        }
        errorCount = android::stats_log_api_gen::write_stats_log_cpp(out, atoms);
        fclose(out);
    }

    // Write the .h file
    if (headerFilename.size() != 0) {
        FILE* out = fopen(headerFilename.c_str(), "w");
        if (out == NULL) {
            fprintf(stderr, "Unable to open file for write: %s\n", headerFilename.c_str());
            return 1;
        }
        errorCount = android::stats_log_api_gen::write_stats_log_header(out, atoms);
        fclose(out);
    }

    // Write the .java file
    if (javaFilename.size() != 0) {
        FILE* out = fopen(javaFilename.c_str(), "w");
        if (out == NULL) {
            fprintf(stderr, "Unable to open file for write: %s\n", javaFilename.c_str());
            return 1;
        }
        errorCount = android::stats_log_api_gen::write_stats_log_java(out, atoms);
        fclose(out);
    }

    // Write the jni file
    if (jniFilename.size() != 0) {
        FILE* out = fopen(jniFilename.c_str(), "w");
        if (out == NULL) {
            fprintf(stderr, "Unable to open file for write: %s\n", jniFilename.c_str());
            return 1;
        }
        errorCount = android::stats_log_api_gen::write_stats_log_jni(out, atoms);
        fclose(out);
    }

    return 0;
}

}
}

/**
 * Main.
 */
int
main(int argc, char const*const* argv)
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    return android::stats_log_api_gen::run(argc, argv);
}
