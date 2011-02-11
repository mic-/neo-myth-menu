//macgen - A generic asm macro preproccesor
//conleon1988@gmail - 2011
//NOTE : THE FOLLOWING CODE IS.... CONCEPT-CODE which means that it lacks PROPER implementation :P
//NOTE : ALL operations are line-based

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string.h>
#include <ostream>
#include <stack>

int TAB_DEPTH = 4;
const char		   PUSH_FSTACK_TOKEN = '>';							//PUSH a function stack element
const char		   POP_STACK_ELEMENT_TOKEN = '%';			//POP a function stack element
const std::string  COMMENT_CHAR = "//";								//COMMENT CHARS
const std::string  MACRO_BEGIN_CODE = ".BEGINMACRO";				//BEGIN MACRO BLOCK
const std::string  MACRO_END_CODE = ".ENDMACRO";					//END MACRO BLOCK
const std::string  REPEAT_BEGIN_CODE = ".BEGINREPEAT";				//BEGIN REPEAT (NOT IMPL)
const std::string  REPEAT_END_CODE = ".ENDREPEAT";					//END REPEAT (NOT IMPL)

struct Macro
{
	std::string name;
	std::vector<std::string> args;
	std::vector<std::string> code;
	unsigned int line;
	unsigned int line_count;
};

static std::vector<Macro> global_macros;

inline unsigned int skip_whitespace(const std::string& s,unsigned int addr)
{
	while(isspace(s[addr]))
		++addr;

	return addr;
}

static Macro* find_macro(const std::string& name)
{
	for(unsigned int i = 0; i < global_macros.size();i++)
	{
		if(global_macros[i].name == name)
			return &global_macros[i];
	}

	return 0;
}

inline bool is_keyword(const std::string& s)
{
	return !((s != MACRO_BEGIN_CODE) && (s != MACRO_END_CODE) 
			&& (s != REPEAT_BEGIN_CODE) && (s != REPEAT_END_CODE));
}

static unsigned int register_macro(std::vector<std::string>& code,unsigned int line)
{	
	Macro mac;
	Macro* pmac;

	if( (pmac = find_macro(mac.name)) != 0)
	{
		std::cout << "Macro '" << mac.name << "' at line " << line << " defined already at line #"<<pmac->line << std::endl;
		exit(1);
	}

	mac.line = line;

	//Parse name
	{
		const std::string& s = code[line];
		int i = s.find(MACRO_BEGIN_CODE);

		if(i == std::string::npos)
		{
			std::cout << "Macro definition at line " << line << " contains no name!"<< std::endl;
			exit(1);
		}
	
		i = skip_whitespace(s,i + MACRO_BEGIN_CODE.length() );

		while(i < s.length())
		{
			if(isspace(s[i]) || s[i] == '(' )
				break;

			mac.name += s[i++];
		}

		if(mac.name.empty())
		{
			std::cout << "Macro definition at line " << line << " contains no name!"<< std::endl;
			exit(1);
		}

		if(is_keyword(mac.name))
		{
			std::cout << "Macro name '" << mac.name << "' at line  #" << line << " contains reserved keyword name!"<< std::endl;
			exit(1);
		}
		
		i = skip_whitespace(s,i);

		if(s[i] != '(')
		{
			std::cout << "Expected '(' after definition of macro '" << mac.name << "' at line #" << line << " but got '" << s[i] << "' " << std::endl;
			exit(1);
		}

		std::cout << "Registered macro '" << mac.name << "'"<<std::endl;

		i = skip_whitespace(s,i + 1);

		if(s[i] == ')')//no args
		{
			//printf("NO ARGs\n");
			//i = skip_whitespace(s,i + 1);
		}
		else//has args
		{
			std::string arg;
			//printf("%s\n",s.c_str());
			while(i < s.length())
			{
				if( ( (s[i] == ')') || (s[i] == '\r') || (s[i] == '\n') || isspace(s[i]) ) )
				{
					if(!arg.empty())
					{
						if(i < s.length()-1)
						{
							if(s[i] == ')' && s[i+1] == ')')
								arg += ')';
							else if(s[i] == ')' && s[i+1] == ',')
							{
								arg += ')';
								mac.args.push_back(arg); //printf("arg %s\n",arg.c_str());
								i += 2;
								arg.clear();
								continue;
							}
						}

						mac.args.push_back(arg); //printf("arg %s\n",arg.c_str());
					}
					break;
				}
				else if((s[i] == ',') && (!arg.empty()) )
				{
					mac.args.push_back(arg); //printf("arg %s\n",arg.c_str());
					arg.clear();
					++i;
					continue;
				}
				
				arg += s[i++];
			}

			i = skip_whitespace(s,i);

			if(s[i] != ')')
			{
				std::cout << "Expected ')' after definition of macro '" << mac.name << "' at line #" << line << std::endl;
				exit(1);
			}
		}
	}

	//Save code
	{
		++line;

		while( (line < code.size()))
		{
			if((int)code[line].find(MACRO_END_CODE) != std::string::npos)
			{
				mac.line_count = (line - mac.line);
				break;
			}
			mac.code.push_back(code[line++]);
		}
		
	}

	global_macros.push_back(mac);

	return line;
}

static unsigned int get_constant(const std::string& s,unsigned int addr,int& res,int& weight)
{
	std::string conv;
	unsigned int save = addr;	
	addr = skip_whitespace(s,addr);
	conv.clear();

	while(addr < s.length())
	{
		if(!isalnum(s[addr]))
			break;
		
		conv += s[addr++];
	}

	if(addr == save)
	{
		std::cout << "Bad constant conversion"<<std::endl;
		exit(1);
	}

	res = atoi(conv.c_str());
	weight = conv.length();
	return skip_whitespace(s,addr);
}

static unsigned int split_cs_args(const std::string& s,std::vector<std::string>& res,std::vector<std::string>& cs)
{
	std::string arg;
	unsigned int i = 0;
	int a = s.find("(");
	bool push_sym = false;

	if(a == std::string::npos)
		a = 0;

	i = skip_whitespace(s,a + 1);
	
	while(i < s.length())
	{
		if(isspace(s[i]))
		{
			if(s[i] == '\r' || s[i] == '\n')
			{
				if(!arg.empty())
				{
					res.push_back(arg); //printf("arg %s\n",arg.c_str());

					if(push_sym)
						cs.push_back(arg);
				}
				break;
			}
			++i;
			continue;
		}
		else if( s[i] == PUSH_FSTACK_TOKEN )//push
		{
			arg.clear();
			push_sym = true;
			++i;
			continue;
		}
		else if( s[i] == POP_STACK_ELEMENT_TOKEN )
		{
			int iconst;
			int weight;
			i = get_constant(s,i + 1,iconst,weight);

			if(cs.empty())
			{
				std::cout << "CS EMPTY can't access stack element #" << iconst << std::endl;
				exit(1);
			}

			res.push_back(cs[iconst]);

			if(push_sym)
			{
				cs.push_back(cs[iconst]);
				push_sym = false;
			}

			arg.clear();
			//i += weight;
			continue;
		}
		else if( s[i] == ')' )
		{
			if(!arg.empty())
			{
				if(i < s.length()-1)
				{
					if(s[i] == ')' && s[i+1] == ')')
						arg += ')';
					else if(s[i] == ')' && s[i+1] == ',')
					{
						arg += ')';
						res.push_back(arg); //printf("arg %s\n",arg.c_str());
						i += 2;

						if(push_sym)
						{
							cs.push_back(arg);
							push_sym = false;
						}

						arg.clear();
						continue;
					}
				}

				res.push_back(arg); //printf("arg %s\n",arg.c_str());

				if(push_sym)
				{
					cs.push_back(arg);
					push_sym = false;
				}

			}
			break;
		}
		else if((s[i] == ',') && (!arg.empty()) )
		{
			res.push_back(arg); //printf("arg %s\n",arg.c_str());

			if(push_sym)
			{
				cs.push_back(arg);
				push_sym = false;
			}
			arg.clear();
			++i;
			continue;
		}
				
		arg += s[i++];
	}
	return skip_whitespace(s,i);
}

static unsigned int call_macro(Macro* mac,std::vector<std::string>& code,unsigned int line,FILE* out,std::vector<std::string> callstack,
const char* parent = 0)
{	
	#define dump_args()\
	{\
		std::cout << "Interface of '" << mac->name << "' : (";\
		for(unsigned int i = 0; i < mac->args.size();i++)\
		{\
			if(i != mac->args.size()-1)\
				std::cout << "<INPUT>" <<mac->args[i] << ",";\
			else \
				std::cout << "<INPUT>" <<mac->args[i] << ")"<<std::endl;\
		}\
	}

	if(!mac)
	{
		std::cout << "Got null MAC" << std::endl;
		exit(1);
	}

	std::cout << "Inlining macro v-call " << mac->name << " @line #" << line << " (depth #" << mac->line_count << ")" << std::endl;   
	const std::string s = ( parent == 0) ? code[line] : parent;

	unsigned int i = skip_whitespace(s,((int)s.find(mac->name) + mac->name.length()) );
	
	if(s[i] != '(')
	{
		std::cout << "Expected '(' after macro call '" << mac->name << "' at line #" << line << " but got '" << s[i] << "' "<<std::endl;
		exit(1);
	}

	i = skip_whitespace(s,i + 1);

	if( (s[i] == ')'))
	{
		if( !mac->args.empty() )
		{
			std::cout << "Macro  '" << mac->name << "' at line #" << line << " takes #" << mac->args.size();
			std::cout << " arguments but got 0 instead!"<< std::endl;
			dump_args();
			exit(1);
		}

		if(!mac->code.empty())
		{
			std::string hint = COMMENT_CHAR;
			hint += "BEGIN macro ";
			hint += mac->name;
			hint += COMMENT_CHAR;
			hint += "\r\n";
			fwrite(hint.c_str(),1,hint.length(),out);
		}
		
		for(unsigned int a = 0; a < mac->code.size();a++)
		{
			bool done = false;
			const std::string& ps = mac->code[a];

			for(unsigned int j = 0; j < global_macros.size();j++)
			{
				if((int)ps.find(global_macros[j].name) != std::string::npos)
				{
					std::vector<std::string> cs;
					call_macro(&global_macros[j],code,line+a,out,cs,ps.c_str());
					done = true;
					break;
				}
			}

			if(done)continue;

			fwrite(mac->code[a].c_str(),1,mac->code[a].length(),out);
		}

		if(!mac->code.empty())
		{
			std::string hint = COMMENT_CHAR;
			hint += "END macro ";
			hint += mac->name;
			hint += COMMENT_CHAR;
			hint += "\r\n";
			fwrite(hint.c_str(),1,hint.length(),out);
		}
	}
	else
	{
		std::vector<std::string> vcall_args;

		i = split_cs_args(s,vcall_args,callstack);

		if(s[i] != ')')
		{
			std::cout << "Expected ')' after macro call '" << mac->name << "' at line #" << mac->line << " but got '" << s[i] <<"' "<< std::endl;
			exit(1);
		}

		if( mac->args.size() != vcall_args.size())
		{
			std::cout << "Macro  '" << mac->name << "' at line #" << line << " takes #" << mac->args.size();
			std::cout << " arguments but got #" << vcall_args.size() << " instead!"<< std::endl;
			dump_args();
			exit(1);
		}

		{
			if(!mac->code.empty())
			{
				std::string hint = COMMENT_CHAR;
				hint += "BEGIN macro ";
				hint += mac->name;
				hint += COMMENT_CHAR;
				hint += "\r\n";
				fwrite(hint.c_str(),1,hint.length(),out);
			}

			for(unsigned int a = 0; a < mac->code.size();a++)
			{
				std::string conv;
				std::string res;
				int b,c,d,e,f;
				const std::string& s = mac->code[a];
				b =
				c =
				d = 
				e =
				f = 0;
				res = "";
				std::vector<std::string> cs;
				
				
				if(callstack.empty())
				{
					
					while ((b = s.find(POP_STACK_ELEMENT_TOKEN,c)) != std::string::npos )
					{
						f = 1;
		
						{
							res += s.substr(c,(b-c)); ++b;

							d = b;

							if(isspace(s[b]))
							{
								std::cout << "Whitespace IS NOT allowed @ argument reference " << "#" << line << std::endl;
								exit(1);
							}

							conv.clear();

							while( (b < s.length()) && isalnum(s[b]) )
								conv += s[b++];

							if(b == d)
							{
								std::cout << "What did you just did THERE : " << "#" << line << " ??? " << std::endl;
								exit(1);
							}

							b = atoi(conv.c_str());

							if(b > vcall_args.size())
							{
								std::cout << "Macro  '" << mac->name << "' at line #" << line << " takes #" << mac->args.size();
								std::cout << " arguments but you tried to reference argument #" << b << std::endl;
								dump_args();
								exit(1);
							}
						
							res += vcall_args[b];
							cs.push_back(vcall_args[b]);
							c = res.length()+conv.length();
							c -= ((((int)vcall_args[b].length())-1) <= 0) ? 0 : (((int)vcall_args[b].length())-1);
						}
					}
				}
				else
				{
					split_cs_args(s,cs,callstack);
					while ((b = s.find(POP_STACK_ELEMENT_TOKEN,c)) != std::string::npos )
					{
						f = 1;
		
						{
							res += s.substr(c,(b-c)); ++b;

							d = b;

							if(isspace(s[b]))
							{
								std::cout << "Whitespace IS NOT allowed @ argument reference " << "#" << line << std::endl;
								exit(1);
							}

							conv.clear();

							while( (b < s.length()) && isalnum(s[b]) )
								conv += s[b++];

							if(b == d)
							{
								std::cout << "What did you just did THERE : " << "#" << line << " ??? " << std::endl;
								exit(1);
							}

							b = atoi(conv.c_str());

							if(b > callstack.size())
							{
								std::cout << "Macro  '" << mac->name << "' at line #" << line << " takes #" << mac->args.size();
								std::cout << " arguments but you tried to reference argument #" << b << std::endl;
								dump_args();
								exit(1);
							}
						
							res += callstack[b];
							cs.push_back(callstack[b]);
							c = res.length()+conv.length();
							c -= ((((int)callstack[b].length())-1) <= 0) ? 0 : (((int)callstack[b].length())-1);
						}
					}					
				}

				bool done = false;

				for(unsigned int j = 0; j < global_macros.size();j++)
				{
					if((int)s.find(global_macros[j].name) != std::string::npos)
					{
						call_macro(&global_macros[j],code,line+a,out,cs,s.c_str());
						done = true;
						break;
					}
				}

				if(done)continue;

				if(!f)
					fwrite(s.c_str(),1,s.length(),out);
				else
				{
					int p = 0;

					while( ((p = res.find(",,",p)) != std::string::npos) && (p < res.length())) //temp hack until i improve the parser :P
					{
						std::string mid = res.substr(0,p+1);
						res = mid + res.substr(p + 2,res.length() );
						p += 2;
					}

					//fix addressing bug due to lame parser ^ :P
					p = s.length()-1;

					//if(res.length() < s.length())
						//res += s[res.length()];

					
					while(p)
					{
						if(s[p] == ',')
							break;
						else if(s[p] == ')')
						{
							if(res[res.length()-1] != ')')
								res += ')';

							break;
						}
						--p;
					}
	

					fwrite(res.c_str(),1,res.length(),out);
					fwrite("\r\n",1,2,out);
				}
			}

			if(!mac->code.empty())
			{
				std::string hint = COMMENT_CHAR;
				hint += "END macro ";
				hint += mac->name;
				hint += COMMENT_CHAR;
				hint += "\r\n";
				fwrite(hint.c_str(),1,hint.length(),out);
			}
		}
	}

	return line;
	#undef dump_args
}

static void erase_all_macro_fields(std::vector<std::string>& code)
{
	std::vector<std::string> res;

	for(unsigned int i = 0; i < code.size();i++)
	{
		bool skip = false;

		for(unsigned int j = 0; j < global_macros.size();j++)
		{
			if(global_macros[j].line == i)
			{
				i += global_macros[j].line_count;
				skip = true;
				break;
			}
		}

		if(!skip)
			res.push_back(code[i]);
	}

	code.clear();
	code = res;
}

static int load_file(const char* filename,std::vector<std::string>& code,const bool kill_comments = false)
{
	FILE* f;
	char buffer[512];

	code.clear();
	f = fopen(filename,"r");

	if(!f)
		return -1;

	while(!feof(f))
	{
		if(fgets(buffer,512,f) == 0)
			break;

		if(kill_comments)
		{
			for(int i = 0; i < COMMENT_CHAR.length();i++)
			{
				if(buffer[i] != COMMENT_CHAR[i])
				{
					code.push_back(buffer);
					break;
				}
			}
		}
		else
			code.push_back(buffer);
	}

	fclose(f);

	return (code.empty()) ? 0 : 1;
}

void dump_src(std::vector<std::string>& code,const char* m = 0)
{
	if(m)std::cout << m << std::endl;

	for(unsigned int i = 0; i < code.size();i++)
	{
		const std::string& s = code[i];
		std::cout << s << std::endl;
	}

	if(m)std::cout << m << std::endl;
}

void parse(std::vector<std::string>& code,const char* dst)
{
	FILE* out = fopen(dst,"w");
	std::vector<std::string> nulli;

	if(!out)
	{
		std::cout << "Failed to open " << dst << std::endl;
		exit(1);
	}

	for(unsigned int i = 0; i < code.size();i++)
	{
		const std::string& s = code[i];

		if((int)s.find(MACRO_BEGIN_CODE) != std::string::npos)
		{
			i = register_macro(code,i);
			continue;
		}
	}

	//dump_src(code);
	erase_all_macro_fields(code);
	//dump_src(code,"=======================");
	for(unsigned int i = 0; i < code.size();i++)
	{		
		const std::string& s = code[i];
		bool done = false;

		for(unsigned int j = 0; j < global_macros.size();j++)
		{
			if((int)s.find(global_macros[j].name) != std::string::npos)
			{
				//printf("FC %s %d :%d\n",global_macros[j].name.c_str(),global_macros.size(),global_macros[j].code.size());
				i = call_macro(&global_macros[j],code,i,out,nulli);
				done = true;
				break;
			}
		}

		if(!done)
		{
			if(code[i].find(MACRO_END_CODE) == std::string::npos)	//extreme case of macros stacked together
				fwrite(code[i].c_str(),1,code[i].length(),out);
		}
	}

	//TODO implement REPEAT
	fclose(out);
}

void fix_ident(const char* source,const bool tabs_to_ws = false,const bool code_hints = false)
{
	std::vector<std::string> code;
	FILE* f;

	std::cout << "Fixing ident of " << source << std::endl;

	const int r = load_file(source,code,code_hints);

	if( r == -1 )
	{
		std::cout << "Failed to open " << source << std::endl;
		exit(1);
	}
	else if( r == 0 )
	{
		std::cout << "File "<< source << " is EMPTY " << std::endl;
		exit(1);
	}

	f = fopen(source,"w");

	for(unsigned int i = 0; i < code.size();i++)
	{
		if(i < code.size()-2)
		{
			const std::string& a = code[i + 0];
			const std::string& b = code[i + 1];

			if((a[0] == '\r' || a[0] == '\n') && (b[0] == '\r' || b[0] == '\n'))
			{
				fwrite("\r\n",1,2,f);
				i += 2;

				while(i < code.size())
				{
					const std::string& s = code[i];

					if((s[0] != '\r') && (s[0] != '\n') )
					{
						--i;
						break;
					}

					++i;
				}
				continue;
			}
		}
		
		if(!tabs_to_ws)
			fwrite(code[i].c_str(),1,code[i].length(),f);
		else
		{
			const std::string& s = code[i];
			std::string final;

			for(unsigned int a = 0; a < s.length();a++)
			{
				if(s[a] == '\t')
				{
					for(int b = 0;b<TAB_DEPTH;b++)
						final += ' ';
				}
				else
					final += s[a];
			}

			fwrite(final.c_str(),1,final.length(),f);
		}
	}

	fclose(f);
}

int main(int argc, char* argv[])
{
	std::vector<std::string> code;
	const char* src = 0;
	const char* dst = 0;
	bool ident = false;
	bool htabws = false;
	bool hints	= true;

	std::cout << "macgen - A generic asm macro preproccesor // conleon1988@gmail - 2011 //" << std::endl;

	if(argc < 3)
	{
		std::cout << "Usage : 		  source.s dest.s" << std::endl;
		std::cout << "Optional args : -ident  (fixes identantion) " << std::endl 
				  << "                -tab2ws (converts hard tabs to whitespace)" << std::endl
				  << "                -wsdepth depth (number of whitespaces in a tab (defaults to 4))" << std::endl
				  << "                -nohints (disables comments/hints in the generated file)" << std::endl;
		return 1;
	}

	src = argv[1];
	dst = argv[2];

	if(argc > 3)
	{
		for(int i = 3;i<argc;i++)
		{
			const char* arg = argv[i];

			if(memcmp(arg,"-ident",strlen("-ident")) == 0)
			{
				std::cout << "macgen :: Will fix ident"<<std::endl;
				ident = true;
			}
			else if(memcmp(arg,"-tab2ws",strlen("-tab2ws")) == 0)
			{
				std::cout << "macgen :: Will convert hard tabs to spaces"<<std::endl;
				htabws = true;
			}
			else if(memcmp(arg,"-nohints",strlen("-nohints")) == 0)
			{
				std::cout << "macgen :: Will disable hints"<<std::endl;
				hints = false;
			}
			else if(memcmp(arg,"-wsdepth",strlen("-wsdepth")) == 0)
			{
				if(++i >= argc)
				{
					std::cout << "macgen :: Missing ws depth arg " << std::endl;
					return 1;
				}

				TAB_DEPTH = atoi(argv[i]);
				std::cout << "macgen :: WS depth was set to :" << TAB_DEPTH << std::endl;
			}
		}
	}

	if( (htabws) && (!ident) )
	{
		std::cout << "macgen :: tab2ws was activated but ident is disabled" << std::endl;
		htabws = false;
	}

	const int res = load_file(src,code,true);
	if( res == -1 )
	{
		std::cout << "Failed to open " << src << std::endl;
		return 1;
	}
	else if( res == 0 )
	{
		std::cout << "File "<< src << " is EMPTY " << std::endl;
		return 1;
	}

	parse(code,dst);

	if(ident)
		fix_ident(dst,htabws,!hints);

	return 0;
}


