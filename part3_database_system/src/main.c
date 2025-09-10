#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "plan.h"
#include "execution.h"
#include "catalog.h"

// --- 临时：用内存数组模拟底层页存储 ---
#include "storage.h"
static Page g_mem[1024]; static int g_used[1024];
page_id_t storage_alloc_page(){ for(int i=0;i<1024;i++) if(!g_used[i]){ g_used[i]=1; return i; } return 0; }
int storage_free_page(page_id_t pid){ g_used[pid]=0; return 0; }
int storage_read_page(page_id_t pid, Page *out){ if(!g_used[pid]) return -1; *out = g_mem[pid]; return 0; }
int storage_write_page(page_id_t pid, const Page *in){ g_used[pid]=1; g_mem[pid]=*in; return 0; }
// ------------------------------------------------

int main(){
    // 引导系统目录页
    for(int i=0;i<1024;i++) g_used[i]=0;
    catalog_bootstrap();

    char line[512];
    printf("minidb > ");
    while (fgets(line, sizeof(line), stdin)){
        if (!strncmp(line,"quit",4)) break;

        PlanNode n={0}; CreateParams cp; InsertParams ip; SelectParams sp; DeleteParams dp;

        if (!strncmp(line,"create",6)){
            // demo: create student(id int, name varchar, age int)
            static const char *cols[]={"id","name","age"};
            static ColType types[]={T_INT,T_VARCHAR,T_INT};
            cp.table_name="student"; cp.ncols=3; cp.col_names=cols; cp.col_types=types;
            n.type=NODE_CREATE; n.params=&cp;
        }else if (!strncmp(line,"insert",6)){
            // demo: insert into student values (1,'Alice',20)
            static const char *vals[]={"1","Alice","20"};
            ip.table_name="student"; ip.ncols=3; ip.col_names=NULL; ip.values_as_text=vals;
            n.type=NODE_INSERT; n.params=&ip;
        }else if (!strncmp(line,"select",6)){
            sp.table_name="student"; sp.nproj=0; sp.proj_cols=NULL; sp.where_expr="age > 18";
            n.type=NODE_SELECT; n.params=&sp;
        }else if (!strncmp(line,"delete",6)){
            dp.table_name="student"; dp.where_expr="id = 1";
            n.type=NODE_DELETE; n.params=&dp;
        }else{
            printf("unknown command. try: create | insert | select | delete | quit\n");
            printf("minidb > "); continue;
        }
        execute_plan(&n);
        printf("minidb > ");
    }
    return 0;
}
