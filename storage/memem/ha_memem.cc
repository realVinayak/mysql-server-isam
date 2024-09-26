#ifdef USE_PRAGMA_IMPLEMENTATION
#pragma implementation // gcc: Class implementation
#endif

#define MYSQL_SERVER 1


#include "sql/sql_class.h"
#include "field_types.h"
#include "sql/field.h"
#include "ha_memem.h"
#include "my_io.h"
#include "mysql/psi/mysql_file.h"

#define MEMEM_NAME_DEXT ".YNV"

#define COUNT_OFFSET sizeof(unsigned int)


static MememDatabase *database;

PSI_file_key mi_key_file_datatmp;

static int memem_table_id(const char *name){
    int i;
    assert(database->tables.size() < INT_MAX);
    for (i = 0; i < (int) database->tables.size(); i++){
        if (strcmp(database->tables[i]->name->c_str(), name) == 0){
            return i;
        }
    }
    return -1;
}

static handler *memem_create_handler(handlerton *hton, TABLE_SHARE *table, bool some [[maybe_unused]], MEM_ROOT *mem_root){
    return new (mem_root) ha_memem(hton, table);
}

static int memem_init(void *p){
    handlerton *memem_hton;

    memem_hton = (handlerton *)p;
    memem_hton->db_type = DB_TYPE_FIRST_DYNAMIC;
    memem_hton->create = memem_create_handler;
    memem_hton->flags = HTON_CAN_RECREATE;

    database = new MememDatabase;
    return 0;
}

static int memem_fini(void *p [[maybe_unused]]){
    delete database;
    return 0;
}

struct st_mysql_storage_engine memem_storage_engine ={
    MYSQL_HANDLERTON_INTERFACE_VERSION
};

mysql_declare_plugin(memem){
    MYSQL_STORAGE_ENGINE_PLUGIN,
    &memem_storage_engine,
    "MEMEM",
    "copied from internet",
    "memory database",
    PLUGIN_LICENSE_GPL,
    memem_init,
    nullptr,
    memem_fini,
    0x0100,
    nullptr,
    nullptr,
    nullptr,
    0
} mysql_declare_plugin_end;


int memem_open_datafile(const char *org_name, int flags){
    char real_data_name[FN_REFLEN];
    fn_format(real_data_name, org_name, "", MEMEM_NAME_DEXT, 4);
    int rc = mysql_file_open(mi_key_file_datatmp, real_data_name, flags, MYF(MY_WME));
    return rc;
}

int ha_memem::create(
    const char *name,
    TABLE *,
    HA_CREATE_INFO *create_info [[maybe_unused]],
    dd::Table *table_def [[maybe_unused]]
){
    assert(memem_table_id(name) == -1);

    auto t = std::make_shared<MememTable>();
    t->name = std::make_shared<std::string>(name);
    database->tables.push_back(t);

    int temp_fd;
    temp_fd = memem_open_datafile(name, O_CREAT);
    assert(temp_fd > 0);

    mysql_file_close(temp_fd, MYF(0));
    return 0;
}

void ha_memem::reset_memem_table(){
    current_position = 0;

    std::string full_name = "./" + std::string(table->s->db.str) + "/" + std::string(table->s->table_name.str);
    int id = memem_table_id(full_name.c_str());
    assert(id >= 0);
    assert(id < (int) database->tables.size());
    memem_table = database->tables[id];
}

int ha_memem::write_row(uchar *buff){
    if (memem_table == NULL){
        reset_memem_table();
    }
    assert(current_fd != 0);

    my_off_t off = my_seek(current_fd, 0, SEEK_END, MYF(0));
    if (off == MY_FILEPOS_ERROR) return ENOMEM;

    size_t wrote = mysql_file_write(current_fd, buff, table_share->reclength, 0);

    assert(wrote == table_share->reclength);

    return 0;
}

int ha_memem::rnd_init(bool scan [[maybe_unused]]){
    reset_memem_table();
    return 0;
}

int ha_memem::rnd_next(uchar *buf){

    row_buff =(uchar *) malloc((sizeof(uchar)*table_share->reclength));

    my_off_t off = my_seek(current_fd, current_position * table_share->reclength, SEEK_SET, MYF(0));
    if (off == MY_FILEPOS_ERROR) return ENOMEM;

    if (!row_buff) return ENOMEM;


    size_t row_from_file = mysql_file_read(current_fd, row_buff, table_share->reclength, 0);

    // we don't want to handle bad rows...
    assert(row_from_file == table_share->reclength || row_from_file == 0);

    if (row_from_file == 0){
        // perform an early free...
        free(row_buff);
        return HA_ERR_END_OF_FILE;
    }

    memcpy(buf, row_buff, table_share->reclength);

    current_position++;
    return 0;
}


int ha_memem::update_row(const uchar *, uchar *){
    return HA_ERR_WRONG_COMMAND;
    // assert(current_position > 0);
    // int pos = current_position - 1;

    // assert(memem_table != NULL);

    // auto row = memem_table->rows[pos];
    // std::copy(new_data, new_data + table_share->reclength, row->data());
    // return 0;
}

int ha_memem::open(const char *name, int , uint , const dd::Table *){
    int temp_fd;
    if (table->s->tmp_table == NO_TMP_TABLE){
        temp_fd = memem_open_datafile(name, O_RDWR | O_CREAT);
        if (temp_fd <= 0){
            return 1;
        }
        current_fd = temp_fd;
    }
    return 0;
}
