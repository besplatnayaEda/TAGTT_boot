
#ifndef  __APP_BUF_INCLUDED
#define  __APP_BUF_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

#include <jendefs.h>
#include <mac_sap.h>
#include <AppApi.h>

#include "jnx_common.h"


#define APP_MAX_MLME_IND 	(4)
#define APP_MAX_MCPS_IND 	(16)
#define MCPS_BUF_MASK 			(APP_MAX_MCPS_IND - 1)
#define MLME_BUF_MASK 			(APP_MAX_MLME_IND - 1)

typedef struct mcps_buf_s
{   int idx;
    int iwr;
	int ird;
	MAC_McpsDcfmInd_s buf[APP_MAX_MCPS_IND];
} mcps_buf_s;

typedef struct mlme_buf_s
{   int idx;
    int iwr;
	int ird;
	MAC_MlmeDcfmInd_s buf[APP_MAX_MLME_IND];
} mlme_buf_s;



ulong app_inp_buf_init(void);
ulong app_inp_buf_init_tx_only(void);

MAC_MlmeDcfmInd_s *mlme_buf_check(void);
MAC_McpsDcfmInd_s *mcps_buf_check(void);
void mlme_buf_free( MAC_MlmeDcfmInd_s *buf );
void mcps_buf_free( MAC_McpsDcfmInd_s *buf );
void process_mlme_input( MAC_MlmeDcfmInd_s *ind );


#endif  /* __APP_BUF_INCLUDED */

