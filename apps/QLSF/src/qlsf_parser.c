/**============================================================================================================================
 * Depedencies inclusion
 * Necessary dependencies should be declared here. Header file should contain as little dependecies declarations as possible
 *=============================================================================================================================*/

#include "fs.h"
#include "qlsf_parser.h"
#include "qlsf_defs.h"
#include "DMX512_fixture_pool.h"

static qlsf_parser_s this;


/**============================================================================================================================
 * Private functions definitions
 * These functions are only accessible from within the file's scope
 *=============================================================================================================================*/

/**
 * Loads header information into the parser instance
 *
 * Data contained within a QLSF file header conveys information regarding
 * the size of each groups of data chunks, thus enabling a parser to lookup
 * for show-relative information fast.
 * @return qlfs_err_e QLFS_OK on success, specific error code otherwise
 * @see qlsf_defs.h for further information regarding error codes
 */
static qlfs_err_e _qlsf_parser_load_header(void){

	qlfs_err_e err = QLFS_OK;

	TCHAR buf[QLSF_HEADER_BYTESIZE];

	f_read(&this._cur_file, buf, QLSF_HEADER_BYTESIZE - 1, NULL);

	return err;

}

/**
 * Loads patch defined within the config file into the DMX51 engine
 *
 * Patch information is conveyed into 6 bytes data chunk:
 * PATCH_CHUNK_ID_HI_INDEX: 	fixture ID MSB
 * PATCH_CHUNK_ID_LO_INDEX: 	fixture ID LSB
 * PATCH_CHUNK_ADDR_HI_INDEX:	fixture address MSB
 * PATCH_CHUNK_ADDR_LO_INDEX:	fixture address LSB
 * PATCH_CHUNK_CHN_HI_INDEX:	fixture channel count MSB
 * PATCH_CHUNK_CHN_LO_INDEX:	fixture channel count LSB
 * @return qlfs_err_e QLFS_OK on success, specific error code otherwise
 * @see qlsf_defs.h for further information regarding error codes
 */
static qlfs_err_e _qlsf_parser_load_fixtures(void){

	qlfs_err_e err = QLFS_OK;

	uint16_t id;
	uint16_t addr;
	uint16_t chn;

	FSIZE_t fsize = f_size(&this._cur_file);
	TCHAR buf[fsize];

	f_read(&this._cur_file, buf, fsize - 1, NULL);

	for(int i=0; i<fsize; i += QLSF_PATCH_CHUNK_BYTESIZE){
		id 	 = buf[i + QLSF_PATCH_CHUNK_ID_HI_INDEX] 	| (buf[i + QLSF_PATCH_CHUNK_ID_LO_INDEX]   << 8);
		addr = buf[i + QLSF_PATCH_CHUNK_ADDR_HI_INDEX] 	| (buf[i + QLSF_PATCH_CHUNK_ADDR_LO_INDEX] << 8);
		chn  = buf[i + QLSF_PATCH_CHUNK_CHN_HI_INDEX] 	| (buf[i + QLSF_PATCH_CHUNK_CHN_LO_INDEX]  << 8);
		DMX512_fixture_pool_add(id, addr, addr + chn - 1);
	}

	return err;

}

/**
 * Loads scenes defined within the config file into the DMX512 engine
 *
 * @return qlfs_err_e QLFS_OK on success, specific error code otherwise
 * @see qlsf_defs.h for further information regarding error codes
 */
static qlfs_err_e _qlsf_parser_load_scenes(void){

	qlfs_err_e err = QLFS_OK;



	return err;
}

static qlfs_err_e _qlsf_parser_load_chasers(void){

	qlfs_err_e err = QLFS_OK;

	return err;

}


/**============================================================================================================================
 * Public functions definitions
 * These functions can be accessed outside of the file's scope
 * @see qlsf_parser.h for declarations
 *=============================================================================================================================*/

/**
 * Initialises the "Q Lightweight Show File" parser
 *
 * @return qlfs_err_e QLFS_OK on success, specific error code otherwise
 * @see qlsf_defs.h for further information regarding error codes
 */
qlfs_err_e qlsf_parser_init(char *file_path){

	qlfs_err_e err = QLFS_OK;

	if(fs_init() != FS_OK){
		err = QLFS_FILE_SYSTEM_ERR;
	}else if(f_open(&this._cur_file, file_path, FA_OPEN_ALWAYS | FA_READ | FA_WRITE) != FR_OK){
		err = QLFS_FILE_NOT_FOUND;
	}else if(_qlsf_parser_load_header()){

	}else if(_qlsf_parser_load_fixtures() != QLFS_OK){
		err = QLFS_PATCH_DEF_ERR;
	}else if(_qlsf_parser_load_scenes() != QLFS_OK){
		err = QLFS_SCENE_DEF_ERR;
	}else if(_qlsf_parser_load_chasers() != QLFS_OK){
		err = QLFS_CHASER_DEF_ERR;
	}else{

	}

	return err;

}
