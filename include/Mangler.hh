#pragma once

#include <vector>

class Mangler {
	using IDS = std::vector<std::string> &;


public:
	/* 	<name> 	::=	<nested> <type>
	 *			::=	<unscoped-name> <type>
	 */
    inline std::string mangle(std::vector<std::string> ids) {
    	if (ids.size() > 1) {
    		this->parseNested(ids);
    	} else {
    		this->parseUName(ids);
    	}
    	this->parseArgs(ids);
    	return _name;
    }

protected:

	void parseArgs(IDS ids) {

	}

	/* 	
	 *	<nested> 	::=	N <prefix> <unscoped-name> E
	 */
	void parseNested(IDS ids) {
		_name += "N";
		this->parsePrefix(ids);
		this->parseUName(ids);
		_name += "E";
	}

	/*
	 *	<prefix>	::=	<unscoped-name>
	 *				::=	<prefix> <unscoped-name>
	 */
	void parsePrefix(IDS ids) {
		while (ids.size() > 1) {
			this->parseUName(ids);
		}
	}

	/*
	 *	<unscoped-name>	::=	<operator-name>
	 *					::=	<ctor-dtor-name>
     *                  ::= <source-name>
	 */
	void parseUName(IDS ids) {
		if ((!this->parseOperator(ids)) && (!this->parseCDtor(ids))) {
			this->parseSourceName(ids);
		}
	}

	/*
	 *	<operator>	::= nw	# new
	 *				::= na	# new[]
	 *	  			::= dl	# delete
	 *	  			::= da	# delete[]    
	 *				::= pl	# +
	 *	  			::= mi	# -
	 *				::= ml	# *
	 *	  			::= dv	# /
	 *	  			::= rm	# %       
	 *	  			::= aS	# =             
	 *	  			::= pL	# +=            
	 *	  			::= mI	# -=            
	 *	  			::= mL	# *=            
	 *	  			::= dV	# /=            
	 *	  			::= rM	# %=                      
	 *	  			::= ls	# <<            
	 *	  			::= rs	# >>            
	 *	  			::= lS	# <<=           
	 *	  			::= rS	# >>=           
	 *	  			::= eq	# ==            
	 *	  			::= ne	# !=            
	 *	  			::= lt	# <             
	 *	  			::= gt	# >             
	 *	  			::= le	# <=            
	 *	  			::= ge	# >=            
	 *	  			::= nt	# !                       
	 *	  			::= pp	# ++ (postfix in <expression> context)
	 *	  			::= mm	# -- (postfix in <expression> context)            
	 *	  			::= pt	# ->            
	 *	  			::= ix	# []            
	 *	  			::= cv <type>	# (cast)
	 */
	bool parseOperator(IDS ids) {
		if (ids.front().substr(0, 8) != "operator") {
			return false;
		}
		auto epur = ids.front().substr(8);
			try {
				std::string ope = operatorMap.at(epur);
				_name += ope;
			} catch (std::out_of_range &) {
				if (epur.size() == 0) return false;
				_name += "cv";
				_name += std::to_string(epur.size()) + epur;
			}
		next(ids);
		return true;
	}

	bool parseCDtor(IDS ids) {
		if (ids.front() == "ctor") {
			_name += "C1";
			next(ids);
			return true;
		} else if (ids.front() == "dtor") {
			_name += "D0";
			next(ids);
			return true;
		}
		return false;
	}

	bool parseSourceName(IDS ids) {
		_name += std::to_string(ids.front().size());
		_name += ids.front();
		next(ids);
		return true;
	}

	void next(IDS ids) {
		ids.erase(ids.cbegin());
	}

protected:
	std::string _name = "_K";
	const std::unordered_map<std::string, const char *> operatorMap = {
		{"new", "nw"},
		{"new[]", "na"},
		{"delete", "dl"},
		{"delete[]", "da"},
		{"+", "pl"},
		{"-", "mi"},
		{"*", "ml"},
		{"/", "dv"},
		{"%", "rm"},
		{"=", "aS"},
		{"+=", "pL"},
		{"-=", "mI"},
		{"*=", "mL"},
		{"/=", "dV"},
		{"%=", "rM"},
		{"==", "eq"},
		{"!=", "ne"},
		{"<", "lt"},
		{">", "gt"},
		{"<=", "le"},
		{">=", "ge"},
		{"!", "nt"},
		{"&&", "aa"},
		{"||", "oo"},
		{"++", "pp"},
		{"--", "mm"}
	};

};
