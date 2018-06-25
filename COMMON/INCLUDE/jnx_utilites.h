

#ifndef __SYSTEM_ADD_JNX_H
#define __SYSTEM_ADD_JNX_H

void AppColdStart(void);
void AppWarmStart(void);

bint init_beacon(void);
void start_energy_scan(void);
void start_coordinator( bint quiet );
void handle_node_association( MAC_MlmeDcfmInd_s *ind );
void handle_energy_scan_response( MAC_MlmeDcfmInd_s *ind );





#endif

