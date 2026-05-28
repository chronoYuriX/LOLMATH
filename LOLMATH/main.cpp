#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdint.h>
#include <tchar.h>
#include <strsafe.h>


typedef float   MATHfloat;
typedef int32_t MATHint;
typedef int8_t  MATHtag;
typedef uint8_t MATHargs;
typedef size_t  MATHsize;
struct MATHraw;
typedef MATHraw (*MATHfunc)(MATHraw*, MATHargs);
struct MATHraw {
    union {
        MATHfloat F;
        MATHint I;
        MATHfunc func;
        void* rawptr;
    };
};
inline MATHraw makeraw(MATHint i) {
    MATHraw temp;
    temp.I = i;
    return temp;
}
inline MATHraw makeraw(MATHfloat f) {
    MATHraw temp;
    temp.F = f;
    return temp;
}

#define genMATHfunc2v(N, T_0, T_1, OP) __fastcall MATHraw N##T_0##T_1(MATHraw* var, MATHargs unused) { \
    return makeraw(var[0].T_0 OP var[1].T_1); }
#define genMATHfunc2v_4(N, OP) genMATHfunc2v(N, I, I, OP) genMATHfunc2v(N, I, F, OP) \
	genMATHfunc2v(N, F, I, OP) genMATHfunc2v(N, F, F, OP)
genMATHfunc2v_4(add, +) genMATHfunc2v_4(sub, -) genMATHfunc2v_4(mul, *) genMATHfunc2v_4(div, /);
genMATHfunc2v(and, I, I, &) genMATHfunc2v(or, I, I, |) genMATHfunc2v(xor, I, I, ^);
__fastcall MATHraw sigmaF(MATHraw* var, MATHargs n) {
    MATHraw res; res.F = 0;
    for (MATHint i = 0; i < n; i++) res.F += var[i].F;
    return res;
}
__fastcall MATHraw sigmaI(MATHraw* var, MATHargs n) {
    MATHraw res; res.I = 0;
    for (MATHint i = 0; i < n; i++) res.I += var[i].I;
    return res;
}

void errW(const wchar_t* errinfo) {
    MessageBoxW(NULL, errinfo, L"ERROR", MB_ICONERROR);
    int result = MessageBoxW(
        NULL, L"Do you want to terminate this program?", L"TERMINATE", MB_ICONWARNING | MB_ABORTRETRYIGNORE
    );
    switch (result) {
        case IDIGNORE: break;
        case IDABORT: exit(1);
        case IDRETRY: {
            wchar_t path[MAX_PATH];
            DWORD len = GetModuleFileNameW(NULL, path, MAX_PATH);
            STARTUPINFOW si = { sizeof(si) };
            PROCESS_INFORMATION pi;
            wchar_t cmd[] = L"-mode=retry";
            CreateProcessW(path, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
            exit(2);
        }
    }
}

struct MATHcontainer {
    MATHsize size_max, size_used;
    float expansion;
    void* data;
    MATHcontainer(MATHsize size, float expansion): size_max(size), expansion(expansion), size_used(0) {
        data = malloc(size);
        if (!data) errW(L"Out of memory!");
        if (expansion <= 1.f) errW(L"Bad expansion rate");
    }
    ~MATHcontainer() {
        if (data) free(data);
    }
    void resize(MATHsize size) {
        size_max = size;
        void* temp = realloc(data, size);
        if (temp) data = temp;
        else errW(L"Out of memory!");
    }
    void expand() {
        MATHsize size_increased = size_max * expansion;
        if (size_increased == size_max) size_increased++;
        resize(size_increased);
    }
    void expandto(MATHsize size) {
        if (size <= size_max) return;
        MATHsize size_increased = size_max;
        while (size_increased < size) {
            MATHsize size_temp = size_increased * expansion;
            if (size_temp > size_increased) size_increased = size_temp;
            else size_increased++;
        }
        resize(size_increased);
    }
    void append(void* item, MATHsize size) {
        expandto(size + size_used);
        memcpy((BYTE*)data + size_used, item, size);
        size_used += size;
    }
    void minimize() {
    	if (size_used < size_max) resize(size_used);
	}
};

struct lsMATHstaticstr {
    MATHcontainer data, strlocs;
    MATHsize counter;
    lsMATHstaticstr(): data(256, 1.3f), strlocs(16, 1.5f), counter(0) { }
    MATHsize addstr(const wchar_t* str) {
        strlocs.append((void*)&(data.size_used), sizeof(MATHsize));
        data.append((void*)str, wcslen(str) * sizeof(wchar_t));
        return counter++;
    }
    void getedge(MATHsize strID, MATHsize* left, MATHsize* right) {
        *left = ((MATHsize*)(strlocs.data))[strID];
        *right = (strID + 1 == counter) ? data.size_used : ((MATHsize*)(strlocs.data))[strID + 1];
    }
    void extract(MATHsize strID, wchar_t* dst) {
        MATHsize left, right;
        getedge(strID, &left, &right);
        MATHsize size = right - left;
        memcpy(dst, (BYTE*)data.data + left, size);
        *(dst + size / sizeof(wchar_t)) = L'\0';
    }
    MATHsize find(const wchar_t* str) {
        for (MATHsize strID = 0; strID < counter; strID++) {
            MATHsize left, right, len = wcslen(str) * sizeof(wchar_t);
            getedge(strID, &left, &right);
            if (right - left == len && memcmp(str, (BYTE*)(data.data) + left, len) == 0) return strID;
        }
        return ~0;
    }
};

const MATHtag MATH_SUCCESS = 0, MATH_FAILED = 1;
struct MATHenv {
    lsMATHstaticstr var_strs;
    MATHcontainer vals;
    MATHraw val_default;
    MATHenv(MATHraw val_default): var_strs(), vals(16, 1.5f), val_default(val_default) { }
    void addvar(const wchar_t* varname, MATHraw val) {
        if (var_strs.find(varname) == ~0) {
            var_strs.addstr(varname);
            vals.append((void*)&val, sizeof(val));
        }
    }
    MATHraw getval(const wchar_t* varname) {
        MATHsize strID = var_strs.find(varname);
        if (strID != ~0) return ((MATHraw*)(vals.data))[strID];
        return val_default;
    }
    bool setval(const wchar_t* varname, MATHraw val) {
        MATHsize strID = var_strs.find(varname);
        if (strID != ~0) {
            ((MATHraw*)(vals.data))[strID] = val;
            return MATH_SUCCESS;
        }
        return MATH_FAILED;
    }
};

const MATHtag MATHNODE_FLOAT = 1, MATHNODE_INT = 2, MATHNODE_FUNC = 3, MATHNODE_FUNC_INTERMID = 4, MATHNODE_VAR = 5;
struct MATHnode {
    MATHtag tag;
    MATHraw rawdata;
    MATHargs paramsize;
    MATHsize memloc;
    MATHnode(MATHfloat val): tag(MATHNODE_FLOAT) { rawdata.F = val; }
    MATHnode(MATHint val): tag(MATHNODE_INT) { rawdata.I = val; }
    MATHnode(MATHfunc func, MATHnode** params, MATHargs paramsize, MATHcontainer* memspace):
            paramsize(paramsize), tag(MATHNODE_FUNC) {
        rawdata.func = func;
        memloc = memspace->size_used;
        for (MATHargs i = 0; i < paramsize; i++) memspace->append((void*)&params[i], sizeof(MATHnode*));
    }
    MATHnode(MATHfunc func, MATHsize* paramlocs, MATHargs paramsize, MATHcontainer* memspace): // use byte offset!
    	    paramsize(paramsize), tag(MATHNODE_FUNC_INTERMID) {
        rawdata.func = func;
        memloc = memspace->size_used;
        for (MATHargs i = 0; i < paramsize; i++) memspace->append((void*)&paramlocs[i], sizeof(MATHsize));
    }
    MATHnode(const wchar_t* varname, MATHcontainer* memspace):
            paramsize(wcslen(varname)), tag(MATHNODE_VAR) {
        memloc = memspace->size_used;
        memspace->append((void*)varname, wcslen(varname) * sizeof(wchar_t));
    }
    MATHraw call(MATHcontainer* memspace, MATHenv* env, MATHcontainer* nodes) {
        switch (tag) {
            case MATHNODE_FLOAT: case MATHNODE_INT: return rawdata;
            case MATHNODE_FUNC: {
                MATHraw* params = (MATHraw*)__builtin_alloca(paramsize * sizeof(MATHraw));
                for (MATHargs i = 0; i < paramsize; i++)
                    params[i] = ((MATHnode**)((BYTE*)(memspace->data) + memloc))[i]->call(memspace, env, NULL);
                return rawdata.func(params, paramsize);
            }
			case MATHNODE_FUNC_INTERMID: {
				MATHraw* params = (MATHraw*)__builtin_alloca(paramsize * sizeof(MATHraw));
				for (MATHargs i = 0; i < paramsize; i++) {
					MATHsize param_offset = ((MATHsize*)((BYTE*)(memspace->data) + memloc))[i];
					params[i] = ((MATHnode*)((BYTE*)(nodes->data) + param_offset))->call(memspace, env, nodes);
				}
				return rawdata.func(params, paramsize);
			}
            case MATHNODE_VAR: {
                MATHsize var_str_size = paramsize * sizeof(wchar_t);
                wchar_t* var_str = (wchar_t*)__builtin_alloca(var_str_size);
                memcpy(var_str, (BYTE*)(memspace->data) + memloc, var_str_size);
                var_str[paramsize] = L'\0';
                return env->getval(var_str);
            }
        }
        return makeraw(0); // Make -Wreturn-type happy
    }
};

struct lsMATHnode {
	MATHcontainer nodes, memspace, trackers;
	MATHenv env;
	MATHargs tracker_counter;
	MATHsize root_offset;
	bool iscompleted;
	lsMATHnode(): nodes(256, 1.3f), memspace(256, 1.3f), trackers(256, 1.5f), env(makeraw(0)), tracker_counter(0),
	        iscompleted(0) { }
	~lsMATHnode() {
		for (MATHargs i = 0; i < tracker_counter; i++) delete ((MATHcontainer**)(trackers.data))[i];
	}
	void track(MATHargs trackerID, MATHsize loc) {
        if (trackerID <= 0) return;
	    if (trackerID <= tracker_counter)
		    ((MATHcontainer**)(trackers.data))[trackerID - 1]->append((void*)&loc, sizeof(MATHsize));
		else if (trackerID == tracker_counter + 1) {
	        MATHcontainer* cur_tracker = new MATHcontainer(64, 2.f);
			trackers.append((void*)&cur_tracker, sizeof(cur_tracker));
			tracker_counter++;
			track(trackerID, loc);
		}
	}
    void addnode(MATHfloat val, MATHargs trackerID) {
    	if (iscompleted) return;
    	track(trackerID, nodes.size_used);
		MATHnode cur_node(val);
		nodes.append((void*)&cur_node, sizeof(cur_node));
	}
    void addnode(MATHint val, MATHargs trackerID) {
    	if (iscompleted) return;
    	track(trackerID, nodes.size_used);
		MATHnode cur_node(val);
		nodes.append((void*)&cur_node, sizeof(cur_node));
	}
	void addnode(const wchar_t* varname, MATHargs trackerID) {
		if (iscompleted) return;
		track(trackerID, nodes.size_used);
		MATHnode cur_node(varname, &memspace);
		nodes.append((void*)&cur_node, sizeof(cur_node));
	}
	void addnode(MATHfunc func, MATHargs using_trackerID, MATHargs trackerID) {
		if (using_trackerID <= 0 || using_trackerID > tracker_counter || iscompleted) return;
		track(trackerID, nodes.size_used);
		MATHcontainer* cur_tracker = ((MATHcontainer**)(trackers.data))[using_trackerID - 1];
		MATHnode cur_node(func, (MATHsize*)(cur_tracker->data), cur_tracker->size_used / sizeof(MATHsize), &memspace);
		nodes.append((void*)&cur_node, sizeof(cur_node));
	}
	void reset_trackers() {
		for (MATHargs i = 0; i < tracker_counter - 1; i++)
			(((MATHcontainer*)(trackers.data) + i))->size_used = 0;
	}
	void finish_build(bool minimize) {
		root_offset = nodes.size_used - sizeof(MATHnode);
		if (minimize) {
			nodes.minimize(); memspace.minimize(); trackers.minimize();
			for (MATHargs i = 0; i < tracker_counter; i++) ((MATHcontainer**)(trackers.data))[i]->minimize();
		}
		iscompleted = 1;
	}
	void rebuild() {
		reset_trackers();
		nodes.size_used = memspace.size_used = 0;
		iscompleted = 0;
	}
	MATHraw call() {
		if (iscompleted) return ((MATHnode*)((BYTE*)(nodes.data) + root_offset))->call(&memspace, &env, &nodes);
		return makeraw(~0);
	}
};

const MATHtag TOKEN_OP = 1, TOKEN_NUM = 2, TOKEN_PARENTHESES = 3;
struct MATHtoken {
    MATHtag tag;
    MATHraw rawdata;
    
}
struct MATHexpression {
	lsMATHnode absyn;
	MATHcontainer exp;
    MATHsize exprlen;
	MATHexpression(): exp(1024, 1.5f), exprlen(0) { }
	void load(const wchar_t* exp_str) {
	    exp.size_used = 0;
        exprlen = wcslen(exp_str);
		exp.append((void*)exp_str, (exprlen + 1) * sizeof(wchar_t));
		parse();
	}
	void setval(const wchar_t* varname, MATHraw val) {
		if (absyn.env.setval(varname, val) == MATH_FAILED) absyn.env.addvar(varname, val);
	}
	void parse() {
		wchar_t* exp_str = (wchar_t*)(exp.data);
		for (MATHsize i = 0; i < exprlen; i++) {

		}
	}
    MATHsize match_parentheses(MATHsize left) {
        wchar_t* exp_str = (wchar_t*)(exp.data);
        MATHsize left_parentheses = 1;
        for (MATHsize i = left + 1; i < exprlen; i++) {
            if (exp_str[i] == L'(') left_parentheses++;
            else if (exp_str[i] == L')') {
                if (left_parentheses == 0) return ~0;
                if (--left_parentheses == 0) return i;
            }
        }
        return ~0;
    }
    bool check_parentheses() {
        wchar_t* exp_str = (wchar_t*)(exp.data);
        MATHsize left_parentheses = 0;
        for (MATHsize i = 0; i < exprlen; i++) {
            if (exp_str[i] == L'(') left_parentheses++;
            else if (exp_str[i] == L')') {
                if (left_parentheses == 0) return ~0;
                if (--left_parentheses == 0) return i;
            }
        }
        return MATH_SUCCESS;
    }
    
};



void printContainerI(MATHcontainer* c) {
    for (MATHsize i = 0; i < c->size_used / sizeof(int); i++)
        wprintf(L"%d\n", ((int*)(c->data))[i]);
}
void printContainerS(MATHcontainer* c) {
    wprintf((wchar_t*)(c->data));
    wprintf(L"\n");
}
void test() {
    {
        MATHcontainer c(10, 1.5f);
        for (int i = 0; i < 100; i++) {
            MATHsize size_fore = c.size_max;
            c.append(&i, sizeof(int));
            if (c.size_max > size_fore) wprintf(L"Extended! %d -> %d\n", size_fore, c.size_max);
        }
        // printContainerI(&c);
    }
    {
        lsMATHstaticstr l;
        l.addstr(L"I am string 1! lol lol lol______123456789ABCDEF");
        l.addstr(L"To be found 000");
        l.addstr(L"I am string 2! 123456789ABCDEF");
        l.addstr(L"To be found 001");
        l.addstr(L"I am string 3! 123456789ABCDEF");
        l.addstr(L"I am string 4! 123456789ABCDEF");
        printContainerS(&(l.data));
        wchar_t buffer[256];
        for (int i = 0; i < l.counter; i++) {
            l.extract(i, buffer);
            wprintf(buffer);
            wprintf(L"\n");
        }
        wprintf(L"ID 0: %d\n", l.find(L"To be found 000"));
        wprintf(L"ID 1: %d\n", l.find(L"To be found 001"));
        wprintf(L"ID ???: %d\n", l.find(L"To be found 002"));
    }
    {
        MATHenv e(makeraw(.114514f));
        e.addvar(L"x", makeraw(1.f));
        e.addvar(L"y", makeraw(2.f));
        e.addvar(L"F**king long name lol man what can I say? ########123456789ABCDEF", makeraw(3.14159265358979f));
        e.addvar(L"Y", makeraw(2.1f));
        printContainerS(&(e.var_strs.data));
        {
            wprintf(L"x's ID = %d\n", e.var_strs.find(L"x"));
            wprintf(L"y's ID = %d\n", e.var_strs.find(L"y"));
            wprintf(L"Y's ID = %d\n", e.var_strs.find(L"Y"));
            wprintf(L"z(undefined)'s ID = %d\n", e.var_strs.find(L"z"));
        }
        wprintf(L"x = %f\n", e.getval(L"x").F);
        wprintf(L"y = %f\n", e.getval(L"y").F);
        wprintf(L"WTF = %f\n", e.getval(L"F**king long name lol man what can I say? ########123456789ABCDEF").F);
        wprintf(L"Y = %f\n", e.getval(L"Y").F);
        wprintf(L"z(undefined) = %f\n", e.getval(L"z").F);
        {
            MATHcontainer memspace(256, 1.3f);
            MATHnode n0(1.f), n1(2.f), n2(3.f), n3(4.f), n4(5.f);
            MATHnode* lsn0[5] = { &n0, &n1, &n2, &n3, &n4 };
            MATHnode n0_1(sigmaF, lsn0, 5, &memspace);
            wprintf(L"Sigma = %f\n", n0_1.call(&memspace, &e, NULL).F, NULL);

            wprintf(L"x = %f\n", e.getval(L"x").F);
            e.setval(L"x", makeraw(10.f));
            wprintf(L"x(changed) = %f\n", e.getval(L"x").F);
            MATHnode n1_1(6.f), n2_1(7.f), n3_1(8.f), n4_1(9.f), n5_1(L"x", &memspace);
            MATHnode* lsn1[6] = { &n0_1, &n1_1, &n2_1, &n3_1, &n4_1, &n5_1 };
            MATHnode n0_2(sigmaF, lsn1, 6, &memspace);
            wprintf(L"Sigma_1 = %f\n", n0_1.call(&memspace, &e, NULL).F, NULL);
            wprintf(L"Sigma_2 = %f\n", n0_2.call(&memspace, &e, NULL).F, NULL);
            e.setval(L"x", makeraw(20.f));
            wprintf(L"Sigma_2(changed) = %f\n", n0_2.call(&memspace, &e, NULL).F);
        }
        {
        	lsMATHnode l;
        	l.addnode(1.f, 1); l.addnode(2.f, 1); l.addnode(3.f, 1);
        	l.addnode(sigmaF, 1, 4);
        	l.finish_build(0);
        	wprintf(L"term 1 finished. result: %f\n", l.call().F);
		}
        {
        	lsMATHnode l;
        	l.addnode(1.f, 1); l.addnode(2.f, 1); l.addnode(3.f, 1);
            l.addnode(4.f, 2); l.addnode(5.f, 2); l.addnode(6.f, 2);
        	l.addnode(7.f, 3); l.addnode(8.f, 3); l.addnode(9.f, 3);
        	l.addnode(sigmaF, 1, 4); l.addnode(sigmaF, 2, 4); l.addnode(sigmaF, 3, 4);
			l.addnode(L"x", 4); l.env.addvar(L"x", makeraw(0.f));
        	l.addnode(sigmaF, 4, 0);
        	l.finish_build(1);
        	wprintf(L"term 2 finished. result: %f\n", l.call().F);
        	l.env.setval(L"x", makeraw(10.f));
        	wprintf(L"result(changed): %f\n", l.call().F);
		}
    }
}

int main() {
    test();
    return 0;
}













