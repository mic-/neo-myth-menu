/*
	map2h - A tool for use with SDCC
	Mic, 2011

	Extracts symbol information from map files and creates an .h file
    with function pointers for easy inter-bank calls
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <iterator>
#include <map>


using namespace std;

vector<string> funcTypes;
vector<string> funcNames;
vector<string> funcArgLists;
vector<string> asmSections[3];
map<string,string> funcsInMapFile;
int blockDepth;
int lineNum = 0;
int column = 0;
int inMultiComment = 0;


bool is_whitespace(char c)
{
	return ((c == ' ') || (c == '\t') || (c == -1));
}


int safe_string_access(string s, int pos)
{
	if ((pos >= 0) && (pos < s.length()))
		return s[pos];
	return -1;
}


// Finds a word (i.e. word=="foo" would match on "foo bar" or "a foo bar" but not on "foobar")
int find_word(string s, const char *word)
{
	int p = s.find(word);
	if (is_whitespace(safe_string_access(s, p - 1)) && is_whitespace(safe_string_access(s, p + strlen(word))))
		return p;
	return string::npos;
}


// Extracts the type of a parameter from a function declaration
// e.g.  "unsigned char * foo" -> "unsigned char *"
string extract_type(string s, int p, int q)
{
    int i, p2;
    int lastWord = -1, currWord, numWords, isPointer, pointerAttachedToName;

    if (p >= q) return "";

    p2 = p;
    if (s.substr(p, q+1-p).find("const") != string::npos)
        p2 = p + s.substr(p, q+1-p).find("const") + 5;
    if (s.substr(p, q+1-p).find("volatile") != string::npos)
        p2 = p + s.substr(p, q+1-p).find("volatile") + 8;
    if (s.substr(p, q+1-p).find("unsigned") != string::npos)
        p2 = p + s.substr(p, q+1-p).find("unsigned") + 8;
    if (s.substr(p, q+1-p).find("signed") != string::npos)
        p2 = p + s.substr(p, q+1-p).find("signed") + 6;

    i = p2;
    numWords = 0;
    isPointer = 0;
    pointerAttachedToName = 0;
    currWord = -1;

    while (i <= q)
    {
        while (is_whitespace(s[i]) && (i <= q)) i++;
        lastWord = currWord;
        currWord = i;
        while ((!is_whitespace(s[i])) && (i <= q)) i++;
        if (i > currWord) numWords++;
        if ((1 == i - currWord) && (s[currWord] == '*')) isPointer = 1;
        else if ((2 == i - currWord) && (s[currWord] == '*') && (s[currWord+1] == '*')) isPointer = 1;
        else if (s[currWord] == '*') pointerAttachedToName = 1;
    }

    if (lastWord != -1) while (!is_whitespace(s[lastWord])) lastWord++;

    //printf("numWords = %d\n", numWords);

    // e.g.  "int"
    if (numWords == 1)
        return s.substr(p, i-p);
    // e.g.  "int *"
    else if ((numWords == 2) && isPointer)
        return s.substr(p, i-p);
    // e.g.  "int *foo"
    else if ((numWords == 2) && !isPointer && pointerAttachedToName)
    {
        if (safe_string_access(s, currWord+1) == '*')
            return s.substr(p, currWord+2-p);
        else
            return s.substr(p, currWord+1-p);
    }
    // e.g. "int foo"
    else if ((numWords == 2) && !isPointer && !pointerAttachedToName)
        return s.substr(p, lastWord-p);
    // e.g. "int * foo"
    else if ((numWords == 3) && isPointer)
        return s.substr(p, lastWord-p);
}


// Is this line a function declaration? If so, extract the function's type, name
// and argument list and save the separately
void check_for_func_decl(string s)
{
	int i, j, p, listStart;
    string argList;

    if (inMultiComment)
    {
        if (s.find("*/") != string::npos) inMultiComment = 0;
    }
    else if ( (s.find("/*") != string::npos) && (s.find("*/") == string::npos) )
    {
        inMultiComment = 1;
    }
    else if ( (find_word(s, "static") == string::npos) &&		// We're not interested in lines that contains any of these words
              (find_word(s, "#define") == string::npos) &&
              (find_word(s, "typedef") == string::npos) &&
              (blockDepth == 0) )
    {
		if (s.find("//") != string::npos) {
			s = s.substr(0, s.find("//"));
		}

        i = 0;
        // Skip past the "extern" qualifier if one is found
        if (find_word(s, "extern") != string::npos) i = find_word(s, "extern") + 6;

        // find the beginning of the first word that makes up the type
        for (; i < s.length(); i++)
        {
            if (!is_whitespace(s[i])) break;
        }

        if ( (s.find("(") != string::npos) &&
    	     (s.find(")") != string::npos) &&
             (s.find(";") != string::npos) &&
             (s.find("(") > i) &&
             (s.rfind(")") > s.find("(")) &&
             (s.find(";") > s.rfind(")")) )
        {
            j = s.find("(") - 1;
            listStart = j + 1;
            p = i;                                  // position of the first word of the type
            while (is_whitespace(s[j])) j--;        // find the end of the last word before the (
            i = j;
            while (!is_whitespace(s[i])) i--;       // find the beginning of the last word before the (

            if (j > i) funcNames.push_back(s.substr(i+1, j-i));
            //printf("Function name is: %s\n", s.substr(i+1, j-i).data());

            while (is_whitespace(s[i])) i--;        // find the end of the second last word before the (
            if (i > p) funcTypes.push_back(s.substr(p, i+1-p));

            argList = "(";

            i = listStart + 1;
            while (i < s.length())
            {
                for (j = i; j < s.length(); j++)
                {
                    if ((s[j] == ';') || (s[j] == ',')) break;
                }
                if (j >= s.length()) break;
                if (s[j] == ';')
                {
                    if (argList.length() > 1) argList += ",";
                    argList += extract_type(s, i, j-2);
                    break;
                }
                // ','
                if (argList.length() > 1) argList += ",";
                argList += extract_type(s, i, j-1);
                i = j + 1;
            }
            argList += ")";
            funcArgLists.push_back(argList);

            //printf("Found a function declaration: %s\n", argList.data());
        }

	}
}


bool is_hex(char c)
{
    if ((c >= '0') && (c <= '9')) return true;
    if ((c >= 'A') && (c <= 'F')) return true;
    if ((c >= 'a') && (c <= 'f')) return true;
    return false;
}


void get_symbol_address(string s)
{
    string address;
    int i,j,k;

    for (i = 0; i < funcNames.size(); i++)
    {
        string sym =  "_" + funcNames[i];
        if (find_word(s, sym.data()) != string::npos)
        {
            j = find_word(s, sym.data());
            while ((j >= 0) && !is_hex(s[j])) j--;
            k = j;
            while ((k >= 0) && is_hex(s[k])) k--;
            address = s.substr(k+1, j-k);

            //printf("Found function in mapfile: %s with address %s\n", sym.data(), address.data());

            funcsInMapFile.insert(pair<string,string>(funcNames[i], address));

            break;
        }
    }
}


int main(int argc, char **argv)
{
	FILE *mapFile, *outHFile, *outCFile, *hFile;
    vector<FILE*> hFiles;
	string oneLine;
    string shortFn,hFileName,cFileName,incFileName;
    map<string,string>::iterator it;
    bool saveCode;
    bool outputConsts = false;
    bool outputAsm = false;
    int symbolOffset = 0;
    bool isSymFile = false;
	int i, j, varOffs, varSize, varsMoved, bytesMoved, ch, currSection;

	if (argc < 3)
	{
		puts("Usage: map2h mapfile hfile [hfile] [hfile] ...");
		return 0;
	}

    shortFn = argv[1];
    if (shortFn.rfind(".") != string::npos)
    {
        if (shortFn.rfind(".") > 0)
            shortFn = shortFn.substr(0, shortFn.rfind("."));
    }
    if (strstr(argv[1], ".sym") != NULL)
        isSymFile = true;

    hFileName = shortFn + "_map.h";
    cFileName = shortFn + "_map.c";
    incFileName = shortFn + "_map.inc";


	// Go through all h-files and look for function declarations
    for (i = 2; i < argc; i++)
    {
        if (strstr(argv[i], ".h") != NULL)
        {
            hFile = fopen(argv[i], "rb");
	        oneLine.clear();
	        while (1)
	        {
		        if ((ch = fgetc(hFile)) != EOF)
		        {
			        if ((ch == 10) || (ch == 13))
			        {
				        lineNum += (ch == 10) ? 1 : 0;
				        check_for_func_decl(oneLine);
				        oneLine.clear();
			        }
			        else
			        {
				        oneLine += (char)ch;
				        if (ch == '{')
					        blockDepth++;
				        else if (ch == '}')
					        blockDepth--;
			        }
		        }
		        else
		        {
			        check_for_func_decl(oneLine);
			        break;
	    	    }
	        }
            fclose(hFile);
        }
        else if (strcmp(argv[i], "--output-consts") == 0)
        {
			outputConsts = true;
		}
        else if (strcmp(argv[i], "--output-asm") == 0)
        {
			outputAsm = true;
		}
    }

	// Now go through the map-file and look up the addresses of the functions
    mapFile = fopen(argv[1], "rb");
    if (mapFile)
    {
	        oneLine.clear();
	        lineNum = 1;
            while (1)
	        {
		        if ((ch = fgetc(hFile)) != EOF)
		        {
			        if ((ch == 10) || (ch == 13))
			        {
                        get_symbol_address(oneLine);
				        lineNum += (ch == 10) ? 1 : 0;
				        oneLine.clear();
			        }
			        else
			        {
				        oneLine += (char)ch;
				        if (ch == '{')
					        blockDepth++;
				        else if (ch == '}')
					        blockDepth--;
			        }
		        }
		        else
		        {
                    get_symbol_address(oneLine);
			        break;
	    	    }
	        }
        fclose(mapFile);
    }
    else
    {
        printf("Error: Unable to open file %s\n", argv[1]);
        return 1;
    }

    if (outputAsm)
    {
        outHFile = fopen(incFileName.data(), "wb");
        if (outHFile)
        {
            fprintf(outHFile, "; Generated by map2h\n");

            for (i = 0; i < funcTypes.size(); i++)
            {
                if (funcsInMapFile.find(funcNames[i]) != funcsInMapFile.end())
                {
                    fprintf(outHFile, "%s = 0x%s\n", funcNames[i].data(), funcsInMapFile.find(funcNames[i])->second.data());
                  }
            }

            fclose(outHFile);
        }
        else
        {
            printf("Error: Unable to create file %s\n", incFileName.data());
            return 1;
        }
    }
    else
    {
        outHFile = fopen(hFileName.data(), "wb");
        if (outHFile)
        {
            fprintf(outHFile, "// Generated by map2h\n");
            fprintf(outHFile, "\n#include \"types.h\"\n\n");

            for (i = 0; i < funcTypes.size(); i++)
            {
                //printf("Index %d, Names %d\n", i, funcNames.size());
                //printf("Looking for %s\n", funcNames[i].data());
                /*if ((it = funcsInMapFile.find(funcNames[i])) != funcsInMapFile.end())
                    fprintf(outHFile, "extern const %s (*pfn_%s)%s;\n", funcTypes[i].data(), funcNames[i].data(), funcArgLists[i].data());*/
                if (funcsInMapFile.find(funcNames[i]) != funcsInMapFile.end())
                {
                    if (outputConsts)
                    {
                        fprintf(outHFile, "const %s (*pfn_%s)%s = (%s (*)%s)0x%s;\n", funcTypes[i].data(), funcNames[i].data(),
                                funcArgLists[i].data(), funcTypes[i].data(), funcArgLists[i].data(),
                                funcsInMapFile.find(funcNames[i])->second.data());
                    }
                    else
                    {
                        fprintf(outHFile, "#define pfn_%s ((%s (*)%s)0x%s)\n", funcNames[i].data(), funcTypes[i].data(),
                                funcArgLists[i].data(), funcsInMapFile.find(funcNames[i])->second.data());
                    }
                }
            }

            fclose(outHFile);
        }
        else
        {
            printf("Error: Unable to create file %s\n", hFileName.data());
            return 1;
        }
    }

/*
    outCFile = fopen(cFileName.data(), "wb");
    if (outCFile)
    {
        fprintf(outCFile, "// Generated by map2h\n");
        fprintf(outCFile, "\n#include <z80/types.h>\n\n");

        for (i = 0; i < funcTypes.size(); i++)
        {
            if (funcsInMapFile.find(funcNames[i]) != funcsInMapFile.end())
                fprintf(outCFile, "const %s (*pfn_%s)%s = (%s (*)%s)0x%s;\n", funcTypes[i].data(), funcNames[i].data(), funcArgLists[i].data(),
                                                                          funcTypes[i].data(), funcArgLists[i].data(), funcsInMapFile.find(funcNames[i])->second.data());
        }

        fclose(outHFile);
    }
    else
    {
        printf("Error: Unable to create file %s\n", cFileName.data());
        return 1;
    }
*/

    funcTypes.clear();
    funcNames.clear();
    funcArgLists.clear();

	return 0;
}
