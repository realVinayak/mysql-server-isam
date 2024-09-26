#ifdef USE_PRAGMA_INTERFACE
#pragma interface /* gcc class implementation */
#endif

#include "thr_lock.h"
#include "sql/handler.h"
#include "sql/table.h"
#include "sql/sql_const.h"

#include <vector>
#include <memory>

typedef std::vector<uchar> MememRow;

struct MememTable{
    std::vector<std::shared_ptr<MememRow>> rows;
    std::shared_ptr<std::string> name;
};

struct MememDatabase{
    std::vector<std::shared_ptr<MememTable>> tables;
};

class ha_memem final : public handler{
    uint current_position = 0;
    std::shared_ptr<MememTable> memem_table = 0;
    int current_fd;
    uchar *row_buff;
    public:
    ha_memem(handlerton *hton, TABLE_SHARE *table_arg): handler(hton, table_arg){
    }
    ~ha_memem() = default;
    const char *table_type() const override {return "Memem";}
    ulonglong table_flags() const override {return HA_NO_AUTO_INCREMENT | HA_NO_TRANSACTIONS;}

    ulong index_flags(uint inx [[maybe_unused]], uint part [[maybe_unused]], bool all_parts [[maybe_unused]]) const override { return 0; }
    #define MEMEM_MAX_KEY MAX_KEY     /* Max allowed keys */
    #define MEMEM_MAX_KEY_SEG 16      /* Max segments for key */
    #define MEMEM_MAX_KEY_LENGTH 3500 /* Like in InnoDB */
    uint max_supported_keys() const override { return MEMEM_MAX_KEY; }
    uint max_supported_key_length() const override { return MEMEM_MAX_KEY_LENGTH; }
    uint max_supported_key_part_length(HA_CREATE_INFO *create_info [[maybe_unused]]) const override { return MEMEM_MAX_KEY_LENGTH; }
    int open(const char *name [[maybe_unused]], int mode [[maybe_unused]], uint open_options [[maybe_unused]], const dd::Table *table_def [[maybe_unused]]) override;
    int close(void) override { return 0; }
    int truncate(dd::Table *table_def [[maybe_unused]]) override { return 0; }
    int rnd_init(bool scan [[maybe_unused]]) override;
    int rnd_next(uchar *buf) override;
    int rnd_pos(uchar *buf [[maybe_unused]], uchar *pos [[maybe_unused]]) override  { return 0; }
    int index_read_map(uchar *buf [[maybe_unused]], const uchar *key [[maybe_unused]], key_part_map keypart_map [[maybe_unused]],
                        enum ha_rkey_function find_flag [[maybe_unused]])
    override {
        return HA_ERR_END_OF_FILE;
    }
    int index_read_idx_map(uchar *buf [[maybe_unused]], uint idx [[maybe_unused]], const uchar *key [[maybe_unused]],
                            key_part_map keypart_map [[maybe_unused]],
                            enum ha_rkey_function find_flag [[maybe_unused]])
    override {
        return HA_ERR_END_OF_FILE;
    }
    int index_read_last_map(uchar *buf [[maybe_unused]], const uchar *key [[maybe_unused]],
                            key_part_map keypart_map [[maybe_unused]])
    override {
        return HA_ERR_END_OF_FILE;
    }
    int index_next(uchar *buf [[maybe_unused]]) override { return HA_ERR_END_OF_FILE; }
    int index_prev(uchar *buf [[maybe_unused]]) override { return HA_ERR_END_OF_FILE; }
    int index_first(uchar *buf [[maybe_unused]]) override { return HA_ERR_END_OF_FILE; }
    int index_last(uchar *buf [[maybe_unused]]) override { return HA_ERR_END_OF_FILE; }
    void position(const uchar *record [[maybe_unused]]) override { return; }
    int info(uint flag [[maybe_unused]]) override { return 0; }
    int external_lock(THD *thd [[maybe_unused]], int lock_type [[maybe_unused]]) override { return 0; }
    int create(const char *name [[maybe_unused]], TABLE *table_arg [[maybe_unused]], HA_CREATE_INFO *create_info [[maybe_unused]], dd::Table *table_def [[maybe_unused]]) override;
    THR_LOCK_DATA **store_lock(THD *thd [[maybe_unused]], THR_LOCK_DATA **to,
                                enum thr_lock_type lock_type [[maybe_unused]])
    override {
        return to;
    }
    int write_row(uchar *buff [[maybe_unused]]) override;
    int update_row(const uchar *old_data [[maybe_unused]], uchar *new_data [[maybe_unused]]) override;
    int delete_row(const uchar *buf [[maybe_unused]]) override {return HA_ERR_WRONG_COMMAND;}


private:
    void reset_memem_table();
};