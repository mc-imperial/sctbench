#ifndef MAPOPTIONS_HXX
#define MAPOPTIONS_HXX

/// \file MapOptions.hxx
/// Defines namespace Convert (to convert between some atomic types and strings)
/// and class MapOptions (to define and access a set of named parameters).
/// 
/// <b>namespace Convert</b> defines conversion functions between atomic <b>int/float/double</b> types and <b>strings</b>.
/// <h2>Example:</h2>
/// \code
/// string pis = "pi is " + Convert::to_string(4 * atan2(1.0, 1.0f));
/// float  pif = Convert::to<float>(pis.c_str() + 6);
/// int    pii = Convert::to<int>  (pis.c_str() + 6);
/// std::cout << "pi is " << pif << "; rounded to " << pii << endl;
/// \endcode

#include <cstring>
#include <vector>
#include <map>

#ifndef DOXYGEN_SHOULD_IGNORE_THIS
#ifdef _MSC_VER
#include <io.h>
#define ACCESS_FILE _access
#define COMPPARE_STRINGS_IGNORE_CASE stricmp
#else
#include <unistd.h>
#define ACCESS_FILE access
#define COMPPARE_STRINGS_IGNORE_CASE strcasecmp
#endif
#endif

#include "RTTL/common/RTVec.hxx"

using namespace std;

/// Defines <b>string <=> atomic types</b> conversions.
namespace Convert {
    _INLINE string to_string(int    v) {char s[12]; sprintf(s, "%-10i "   , v); *strchr(s,' ') = 0; return string(s); }
    _INLINE string to_string(float  v) {char s[16]; sprintf(s, "%-14.7g " , v); *strchr(s,' ') = 0; return string(s); }
    _INLINE string to_string(double v) {char s[24]; sprintf(s, "%-22.14g ", v); *strchr(s,' ') = 0; return string(s); }
    _INLINE string to_string(const string& s) { return s; }
    template<typename DataType> _INLINE DataType to(const string& s)  { 
        cerr << "No default converter...\n"
             << "This is most likely happen when char* values\n"
             << "are assigned to (should be const char*).\n\n";
        exit(1);
        return DataType(0); 
    }
    template<> _INLINE int         to<int   >(const string& s)        { return atoi(s.c_str()); }
    template<> _INLINE float       to<float >(const string& s)        { return float(atof(s.c_str())); }
    template<> _INLINE double      to<double>(const string& s)        { return atof(s.c_str()); }
    template<> _INLINE const char* to<const char*>(const string& s)   { return s.c_str(); }
    template<> _INLINE string      to<string>(const string& s)        { return s; }
};

typedef vector<string> vector_of_strings;
typedef map<string, vector_of_strings*> map_strings_to_vector_of_strings;

/// \class MapOptions
/// Defines a set of the named vectors of strings (parameters) and
/// methods to access them.
///
/// Implementation details (see <em><b>example below</b></em>):
/// <ul>
/// <li> In order to access parameters, they must be parsed (with parse()) or added (with add()) first.
/// <li> parse() processes the second argument argv as a list of the named tokens.
/// <li> "name" of the parameter is defined by the first character(s) which is either
/// <ol>
/// <li> '-' or '--'. In this case, the new parameter replaces the old one with the same name (if it exists).
/// <li> '+'. In this case, the new parameter is appended to the vector of parameters with the same name.
/// </ol>
/// <li> Any token, which does not represent a name (with prefix), is considered to be an entry
/// and appended to the previously named parameter, thus creating potentially infinite vector of
/// parameters with the same name (different entries are accessed using index argument in get() function).
/// <li> Different tokens are either specified as separate entries in argv array or separated with ',' or ';' inside
/// one argv[i] entry.
/// <li> Extra brackets ('[' and ']') could be specified for readability, which are ignored by the parser.
/// <li> <b>There is one exception to this scheme. All tokens with '.' are considered to be filenames
/// and added to the "files" group. If the specified file does not exist, parse() returns false immediately.</b>
/// <li> <b>Additionally, all files with *.ini extension assumed to include parameters and recursively loaded.</b>
/// </ul>
///
/// <b>*.ini</b> file format allows to specify different parameters on different lines.
/// Additionally, nested parameter files could be loaded with the 'include' directive.
///
/// It is allowed to specify component name using [prefixed] notation.
/// Accordingly, the following fragment
/// <pre>
///[camera]
///	pos	= -2.9 1.5 -2.2
///	dir	= 2.9,-0.5,2.2
///	up	= 0; 1; 0 
/// </pre>
/// defines "camera.pos", "camera.dir", and "camera.up" parameters
/// (note different ways of defining the vectors).
///
/// The parsed parameters could be retrieved with the templated get() function,
/// which defines the return data type by the default parameter.
/// \note If the named parameter does not exist, attempt is made to
/// retrieve this parameter from the environment. 
/// 
/// \note MT support exists insofar STL supports it. Accessing/updating vectors simultaneously 
/// may crush the system (read-only multiple accesses are safe, I think :).
/// 
/// <h2>Example for the following command line parameters:</h2>
/// <pre>
/// echo > head.obj; echo > body.obj
/// bin/TestOptions -pbo=1 -pos [1, 2, 3] -up 0,0,1 +dir 5 +dir 6 +dir 7 -nthreads 3 -verb head.obj body.obj
/// </pre>
/// \code
/// #include "RTTL/common/MapOptions.hxx"
/// int main(int argc, const char* argv[]) {
///	// Read parameters from the command line and print them out.
///	options.parse(argc-1, argv+1);
///	cout << options << endl << endl;
///	
///	// Get environment value (defined in Windows and Linux as well).
///	// Note: Under IDE with icc, it will not be defined (as a matter of fact),
///	// but it will be defined if compiled using mvsc.
///	cout << "USERNAME = " << options.get("USERNAME") << endl;
///	// Try to get the value from the environment. get<> type is defined by defvalue.
///	cout << "PROCESSOR_LEVEL = " << options.get("PROCESSOR_LEVEL", -1) << endl;
///
///	cout << "PBO = " << options.get("pbo", 0) << endl;
///	cout << "output is " << (options.defined("verbose, verb, all")? "verbose":"compact") << endl;
///	cout << "pos is [" 
///		 << options.get("pos", 0.0f, 0) << "," 
///		 << options.get("pos", 0.0f, 1) << "," 
///		 << options.get("pos", 0.0f, 2) << "]" << endl;
///	(*options["up"])[1] = Convert::to_string(-1); // change 2nd entry
///	cout << "up  is " << options.getVec3f("up") << endl;
///	cout << "dir is " << options.getVec3f("dir") << endl;
///	cout << options.get("nthreads") << endl; // returns string("3")
///	options.remove("nthreads");
///	cout << options.get("nthreads") << endl; // returns the name
///	int nfiles = options.vector_size("files");
///	for (int fi = 0; fi < nfiles; fi++) {
///		const string& fn = (*options["files"])[fi];
///		cout << "Adding obj file: " << fn << endl;
///	}
/// }
/// \endcode
/// <h2>Output (on Linux machine):</h2>
/// <pre>
///       dir = [5, 6, 7];
///     files = [head.obj, body.obj];
///  nthreads = 3;
///       pbo = 1;
///       pos = [1, 2, 3];
///        up = [0, 0, 1];
/// 
/// USERNAME = aresheto
/// PROCESSOR_LEVEL = -1
/// PBO = 1
/// output is verbose
/// pos is [1,2,3]
/// up  is [0,-1,1]
/// dir is [5,6,7]
/// 3
/// nthreads
/// Adding obj file: head.obj
/// Adding obj file: body.obj
/// </pre>
/// 
/// <h2>Q&A</h2>
/// <ul>
/// <li> Is -pbo the same as --pbo ?
/// - Yes.
/// <li> what's the syntax of specifying an options to a parameter:
/// --pbo=on", or "--pbo=true", or "--pbo=1", or no "=" sign at all, or
/// "--pbo means true, --pbo=0 means off" or "--pbo/--no-pbo" (icc-style)
/// <ol>
/// - Sign ('=') is optional.
/// - Any "pbo" definition will result in defined("pbo") returning true, but its value could be different, depending on comand line parameter.
/// - --pbo=0 will just mean that "pbo" is defined and its value is "0".
/// Then query options.get("pbo", 1) will return integer 0, that's all.</ol>
/// <li>what's the preference of arguments: if two instances of same
/// parameter are specified on cmd line, I assume it's the one the appears
/// last that counts ... is that so ? what about env-vars -- do they
/// _override_ cmd line vars, or is it the other way around ?
/// - Last parameter (either on command line or in ini file overrides previous one with the same name,
/// unless it is prefixed with '+'. In that case the new value will be added to the named vector.
/// Environment has the lowest priority; it is querried only if the value was not specified on command line 
/// or in included file(s).
///
/// <li> Can we add a specific file name extension that's being parsed as if all it's lines are specified in the command line ? i.e., if we create a file default.rtopt (or whatever extension we pick...) with content
/// <pre>
/// res X Y
/// pbo off
/// </pre>
/// then that's the same as having specified "--res X Y --pbo=off" on the command line ?
/// - For whom do you think documentation is written for? Just go and read it :)
/// </ul>

/// Declares mapping of strings to vector of strings, what did you expect?
class MapOptions: public map_strings_to_vector_of_strings {
public:

    // Default ctor.

    /// Dtor.
    ~MapOptions() {
        clear();
    }

	/// clean everything (it is not required if dtor is called implicitly)
    _INLINE void clear() {
        remove_all_entries();
        map_strings_to_vector_of_strings::clear();
    }

	/// Add converted value to the named parameter
    template<typename DataType> 
    _INLINE void add(const string& name, const DataType value) {
        add_string(name, Convert::to_string(value));
    }

	/// Add value to the named parameter
    _INLINE void add_string(const string& name, const string& value) {
        (*this)[name]->push_back(value);
    }

	/// Return 1st entry (with index 0) of the named parameter as a string
	/// or return name as a string if it does not exist (even in the environment).
    _INLINE string get(string name) const {
        return get(name, name, 0);
    }
	/// Return the named vector using defvalue for missing components.
    template<int N, typename DataType>
    _INLINE RTTL::RTVec_t<N, DataType> getVector(string name, DataType defvalue = 0) const {
        RTTL::RTVec_t<N, DataType> v;
        getArray<N, DataType>(name, v, defvalue);
        return v;
    }
    /// Specialization <3, float>.
    _INLINE RTTL::RTVec3f getVec3f(string name, float defvalue = 0) const {
        return getVector<3, float>(name, defvalue);
    }
    /// Specialization <3, int>.
    _INLINE RTTL::RTVec3f getVec3i(string name, float defvalue = 0) const {
        return getVector<3, int>(name, defvalue);
    }
    /// Specialization <2, float>.
    _INLINE RTTL::RTVec2f getVec2f(string name, float defvalue = 0) const {
        return getVector<2, float>(name, defvalue);
    }
    /// Specialization <2, int>.
    _INLINE RTTL::RTVec2i getVec2i(string name, float defvalue = 0) const {
        return getVector<2, int>(name, defvalue);
    }

	/// Write the named vector to vtgt (no more than N entries) or use defvalue
    /// if it is smaller than N.
    template<int N, typename DataType>
    _INLINE void getArray(string name, DataType* vtgt, DataType defvalue = 0) const {
        int n = vector_size(name);
        int i = 0;
        if (n) {
            const vector_of_strings& vs = *(*this)[name];
            for (vector_of_strings::const_iterator it = vs.begin(); it != vs.end(); it++)
                vtgt[i++] = Convert::to<DataType>(*it);
        }
        for (; i < N; i++) {
            vtgt[i] = defvalue;
        }
    }

	/// return index entry in the named parameter if it exists or
	/// defvalue otherwise.
    template<typename DataType> 
    _INLINE DataType get(const string& name, DataType defvalue, unsigned int index = 0) const {
        const_iterator it = find(name);
        if (it == end()) {
            // See if it is defined in the environment...
            const char* parenv = getenv(name.c_str());
            if (parenv) {
                return Convert::to<DataType>(parenv);
            }
            // Nope, return caller-supplied default value.
            return defvalue;
        } else {
            const vector_of_strings& entry = *it->second;
            if (index >= entry.size()) return defvalue;
            return Convert::to<DataType>(entry[index]);
        }
    }

	/// Return # of components in the named vector or 0.
    _INLINE unsigned int vector_size(const string& name) const {
        const_iterator it = find(name);
        if (it == end()) {
            return 0;
        } else {
            vector_of_strings& entry = *it->second;
            return entry.size();
        }
    }

	/// Return true iff the parameter defined by len characters of name exists either in this or in the environment.
    _INLINE bool defined(const char* name, int len) const {
        return defined(string(name, len));
    }
	/// Return true iff the named parameter exists either in this or in the environment.
    /*_INLINE*/ bool defined(const string& name) const {
        const char* ns = name.c_str();
        const char* del = ",; \t";
        if (strcspn(ns, del) == strlen(ns))
            return find(name) != end();

        // Name includes multiple terms: check for each one individually.
        const char* pname = ns;
        do {
            int sep = strcspn(pname, del);
            if (defined(pname, sep))
                return true;
            pname += sep;
            pname += strspn(pname, del);
        } while (*pname && *pname != '\n');

        return false;
    }

	/// Remove the named parameter
    _INLINE void remove(const string& name) {
        // Remove named entry.
        iterator it = find(name);
        if (it != end())
            erase(it);
    }

	/// Parse named tokens defined by argv (see the detailed class description).
    bool parse(int argc, char* argv[]) {
        // Just to allow using main() parameters directly without 
		// any stupid warnings.
        return parse(argc, (const char**)argv);
    }
	/// Parse named tokens defined by a.
    bool parse(const char* a) {
        const char* argv[] = {a};
        return parse(1, argv);
    }

    /// Return true iff all characters of s before separator (" \t,;])" or 0) represents a number 
    /// (using integer, float or exponential notation).
    static bool isNumber(const char* s) {
        // I'll be damned -- parsing a number is not so easy.
        int nss = 0, ndd = 0, nee = 0;
        int n = 0;
        do {
            int ns = s[n] == '-' || s[n] == '+';
            // Signs are either first chars or after exponent.
            if (ns && n && s[n-1] != 'e' && s[n-1] != 'E') return false;
            // Need a digit after the sign.
            if (ns && !(s[n+1] >= '0' && s[n+1] <= '9')) return false;
            nss += ns;
            int nd = s[n] == '.';
            // Need either digit or EOS or exponent after the dot.
            if (nd && !(s[n+1] == 0 || s[n+1] == 'e' || s[n+1] == 'E' || (s[n+1] >= '0' && s[n+1] <= '9'))) return false;
            // No dots after exponent.
            if (nd && nee) return false;
            ndd += nd;
            // No more than 2 signs and one dot.
            if (nss > 2 || ndd > 1) return false;
            int ne = s[n] == 'e' || s[n] == 'E';
            nee += ne;
            // Only one exponent; could not be the first char.
            if (nee > 2 || (ne && !n)) return false;
            if (ne) {
                n++;
                if (s[n] == 0 || strchr(" \t,;])", s[n]))  return false;
                continue;
            }
            if (!((s[n] >= '0' && s[n] <= '9') || ns || nd))
                return false;
            n++;
        } while (s[n] && !strchr(" \t,;])", s[n]));
        return true;
    }

	/// Parse named tokens defined by argv (see the detailed class description).
    bool parse(int argc, const char* argv[]) {
        const char* name = 0;
        const char* ptr;
        int added = 0;
        int  namelen;
        for (int count = 0; count < argc; count++) {

            const char* arg = argv[count];
            if (isNumber(arg)) {
                if (!name) {
                    printf("Unnamed number: %s\n", arg);
                    return false; // report failure
                }
                goto parse_parameters;
            }

            if (arg[0] == '-' || arg[0] == '+') {
                // It is named parameter.
                if (name && !added) {
                    // If the previous parameter does not have any value, just add it with the default value of "1".
                    add(name, "1");
                }

                bool accumulate = (arg[0] == '+');

                // Get name of the current parameter.
                name = arg + 1;
                while (*name == '-') name++; // take care of --parname
                added = 0;
                namelen = strcspn(name, "=");

                if (!accumulate) {
                    // Delete all previous name entries to allow overloading parameters with latter values.
                    remove(string(name, namelen));
                }

                if (namelen == strlen(name))
                    continue;

                // fused name=value
                arg  = name + namelen + 1;
                arg += strspn(arg, " \t"); 
                goto parse_parameters;

            } else if ((ptr = strrchr(arg, '.'))) {
                // This is not a parameter or number or parameter's name; assume that
                // it is a file and check if file exists.
                if (ACCESS_FILE(arg, 0) == -1) {
                    printf("File %s does not exist.\n", arg);
                    return false; // file do not exist; report failure
                }
                // It is a filename. Check if it contains parameters (*.ini).
                if (COMPPARE_STRINGS_IGNORE_CASE(ptr+1, "ini") == 0) {
                    if (parse_file(arg) == false)
                        return false;
                } else {
                    // It is a model file; include it into "files".
                    add("files", arg);
                }
                name = 0; // need a new name

            } else if (name) {
            parse_parameters:
                // Add named parameter. Vectors are allowed, like
                // -camera.pos 1.0 2.0 3.0

                char term[80];
                // Remove prefixed separators if they exist (commas, =, or brackets).
                bool remove_separator = (*arg == '=' || *arg == '(' || *arg == '[' || *arg == ',' || *arg == ';');
                strncpy(term, arg + (remove_separator? 1:0), sizeof(term));
                int remain = strlen(term);
                char* pterm = term;

                // Take apart fused expressions like 1,2,3 and convert them to a vector.
            unfuse:
                int term_end = strcspn(pterm, ",;])");
                pterm[term_end] = 0;

                if (*pterm == 0)
                    continue;

                ++added;
                add(string(name, namelen), pterm);
                pterm  += term_end + 1;
                remain -= term_end + 1;
                if (remain > 0) goto unfuse;

            } else {
                return false; // wrong (unnamed) parameter; report failure
            }
        }
        if (name && !added) {
            // If the last parameter does not have any value, just add it with the default value of "1".
            add(name, "1");
        }

        return true;
    }
	
	/// Parse all named tokens in file filename (see the detailed class description).
    bool parse_file(const char* filename) {
        FILE* fs = fopen(filename, "rt");
        if(fs == 0) {
            printf("File %s does not exist.\n", filename);
            return false;
        }

        char buf[300];
        char prefix[80];
        prefix[0] = 0;

        // ==============================================================================
        // Parse lines like
        // pname = pvalue
        // or
        // pname = [v1, v2, v3] or similar.
        // ==============================================================================
        while(fgets(buf, sizeof(buf), fs)) {
            char* pname = buf + strspn(buf, " \t\n");   // skip blank chars
            if (*pname == 0 || *pname == '/') continue; // commented or empty line

            if (*pname == '[') {
                // [section] will be prepended to all following parameters.
                strncpy(prefix, pname+1, sizeof(prefix));
                int prefixend = strcspn(prefix, "] \t");
                prefix[prefixend] = 0;
                continue;
            }

            bool accumulate = *pname == '+';
            if (accumulate) pname++;

            int name_end = strcspn(pname, "= \t");
            pname[name_end] = 0;

            char* pvalue = pname + name_end + 1;

            bool first_token = true;
            string token_name = *prefix? string(prefix) + string(".") + string(pname) : string(pname);
            if (!accumulate) {
                // Delete all previous name entries to allow overloading parameters with latter values.
                remove(token_name);
            }

            // Loop over multiple values (in vector).
        next_value:
            pvalue += strspn(pvalue, "=,;[( \t\n");
            if (*pvalue == 0) continue;

            int value_end;
            if (*pvalue == '\"') {
                // "value inside"
                value_end = strcspn(++pvalue, "\"\n");
            } else {
                // look for separators
                value_end = strcspn(pvalue, ",;]) \t\n");
            }
            pvalue[value_end] = 0;

            if (first_token) {
                if (COMPPARE_STRINGS_IGNORE_CASE(pname, "include") == 0 || 
                    COMPPARE_STRINGS_IGNORE_CASE(pname, "#include") == 0) {
                    // Parse nested file.
                    if (!parse_file(pvalue)) {
                        return false;
                    }
                    continue;
                }
                if (*pname == '#') continue; // skip comments other than #include
                first_token = false;
            }

            add(token_name, pvalue);

            // Look for the next value (for vectors) if it exists.
            pvalue += value_end + 1;
            goto next_value;
        }

        fclose(fs);
        return true;
    }

    /// const version.
    _INLINE const vector_of_strings* operator[](const string& name) const {
        const_iterator it = find(name);
        if (it == end())
            return 0;
        return it->second;
    }
	/// Return either pointer to the vector_of_strings defined by the name or pointer to the newly created empty vector.
    _INLINE vector_of_strings* operator[](const string& name) {
        vector_of_strings* entry;
        iterator it = find(name);
        if (it == end()) {
            entry = new vector_of_strings;
            insert(value_type(name, entry));
        } else {
            entry = it->second;
        }
        return entry;
    }

protected:
    void remove_all_entries() { /// iterate, clear and delete all entries.
        for (iterator it = begin(); it != end(); it++) {
            vector_of_strings* entry = it->second;
            entry->clear();
            delete entry;
        }
    }

};


/// Output all entries in mo to out in nice format.
_INLINE ostream& operator<<(ostream& out, const MapOptions& mo) {
    MapOptions::const_iterator it;
    unsigned int maxwidth = 0;
    for (it = mo.begin(); it != mo.end(); it++) {
        maxwidth = max(maxwidth, (unsigned int)it->first.length());
    }
    maxwidth++;
    for (it = mo.begin(); it != mo.end(); it++) {
        const vector_of_strings& vec = *it->second;
        unsigned int sz = vec.size();
        cout.width(maxwidth);
        out << it->first << " = ";
        if (sz > 1) out << "[";
        unsigned int i = 0;
        while (true) {
            out << vec[i];
            if (++i == sz) break;
            out << ", ";
        }
        if (sz > 1) out << "]";
        out << ";" << endl;
    }

    return out;
}

/// Allows access to this variable defined in MapOptions.cxx.
extern MapOptions options;

#endif
