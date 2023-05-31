












































static int max_nullfunc_tries = 2 ; 
module_param ( max_nullfunc_tries , int , 0644 ) ; 
module_parm_desc ( max_nullfunc_tries , 
"maximum nullfunc tx tries before disconnecting (reason 4)." ) ; 

static int max_probe_tries = 5 ; 
module_param ( max_probe_tries , int , 0644 ) ; 
module_parm_desc ( max_probe_tries , 
"maximum probe tries before disconnecting (reason 4)." ) ; 









static int beacon_loss_count = 7 ; 
module_param ( beacon_loss_count , int , 0644 ) ; 
module_parm_desc ( beacon_loss_count , 
"number of beacon intervals before we decide beacon was lost." ) ; 











static int probe_wait_ms = 500 ; 
module_param ( probe_wait_ms , int , 0644 ) ; 
module_parm_desc ( probe_wait_ms , 
"maximum time(ms) to wait for probe response" 
" before disconnecting (reason 4)." ) ; 












static u16 
ieee80211_extract_dis_subch_bmap ( const struct ieee80211_eht_operation * eht_oper , 
struct cfg80211_chan_def * chandef , u16 bitmap ) 
{ 
struct ieee80211_eht_operation_info * info = ( void * ) eht_oper -> optional ; 
struct cfg80211_chan_def ap_chandef = * chandef ; 
u32 ap_center_freq , local_center_freq ; 
u32 ap_bw , local_bw ; 
int ap_start_freq , local_start_freq ; 
u16 shift , mask ; 

if ( ! ( eht_oper -> params & ieee80211_eht_oper_info_present ) || 
! ( eht_oper -> params & 
ieee80211_eht_oper_disabled_subchannel_bitmap_present ) ) 
return 0 ; 


ieee80211_chandef_eht_oper ( eht_oper , true , true , & ap_chandef ) ; 
ap_center_freq = ap_chandef . center_freq1 ; 
ap_bw = 20 * bit ( u8_get_bits ( info -> control , 
ieee80211_eht_oper_chan_width ) ) ; 
ap_start_freq = ap_center_freq - ap_bw / 2 ; 
local_center_freq = chandef -> center_freq1 ; 
local_bw = 20 * bit ( ieee80211_chan_width_to_rx_bw ( chandef -> width ) ) ; 
local_start_freq = local_center_freq - local_bw / 2 ; 
shift = ( local_start_freq - ap_start_freq ) / 20 ; 
mask = bit ( local_bw / 20 ) - 1 ; 

return ( bitmap >> shift ) & mask ; 
} 





static void 
ieee80211_handle_puncturing_bitmap ( struct ieee80211_link_data * link , 
const struct ieee80211_eht_operation * eht_oper , 
u16 bitmap , u64 * changed ) 
{ 
struct cfg80211_chan_def * chandef = & link -> conf -> chandef ; 
u16 extracted ; 
u64 _changed = 0 ; 

if ( ! changed ) 
changed = & _changed ; 

while ( chandef -> width > nl80211_chan_width_40 ) { 
extracted = 
ieee80211_extract_dis_subch_bmap ( eht_oper , chandef , 
bitmap ) ; 

if ( cfg80211_valid_disable_subchannel_bitmap ( & bitmap , 
chandef ) ) 
break ; 
link -> u . mgd . conn_flags |= 
ieee80211_chandef_downgrade ( chandef ) ; 
* changed |= bss_changed_bandwidth ; 
} 

if ( chandef -> width <= nl80211_chan_width_40 ) 
extracted = 0 ; 

if ( link -> conf -> eht_puncturing != extracted ) { 
link -> conf -> eht_puncturing = extracted ; 
* changed |= bss_changed_eht_puncturing ; 
} 
} 











static void run_again ( struct ieee80211_sub_if_data * sdata , 
unsigned long timeout ) 
{ 
sdata_assert_lock ( sdata ) ; 

if ( ! timer_pending ( & sdata -> u . mgd . timer ) || 
time_before ( timeout , sdata -> u . mgd . timer . expires ) ) 
mod_timer ( & sdata -> u . mgd . timer , timeout ) ; 
} 

void ieee80211_sta_reset_beacon_monitor ( struct ieee80211_sub_if_data * sdata ) 
{ 
if ( sdata -> vif . driver_flags & ieee80211_vif_beacon_filter ) 
return ; 

if ( ieee80211_hw_check ( & sdata -> local -> hw , connection_monitor ) ) 
return ; 

mod_timer ( & sdata -> u . mgd . bcn_mon_timer , 
round_jiffies_up ( jiffies + sdata -> u . mgd . beacon_timeout ) ) ; 
} 

void ieee80211_sta_reset_conn_monitor ( struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 

if ( unlikely ( ! ifmgd -> associated ) ) 
return ; 

if ( ifmgd -> probe_send_count ) 
ifmgd -> probe_send_count = 0 ; 

if ( ieee80211_hw_check ( & sdata -> local -> hw , connection_monitor ) ) 
return ; 

mod_timer ( & ifmgd -> conn_mon_timer , 
round_jiffies_up ( jiffies + ieee80211_connection_idle_time ) ) ; 
} 

static int ecw2cw ( int ecw ) 
{ 
return ( 1 << ecw ) - 1 ; 
} 

static ieee80211_conn_flags_t 
ieee80211_determine_chantype ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_link_data * link , 
ieee80211_conn_flags_t conn_flags , 
struct ieee80211_supported_band * sband , 
struct ieee80211_channel * channel , 
u32 vht_cap_info , 
const struct ieee80211_ht_operation * ht_oper , 
const struct ieee80211_vht_operation * vht_oper , 
const struct ieee80211_he_operation * he_oper , 
const struct ieee80211_eht_operation * eht_oper , 
const struct ieee80211_s1g_oper_ie * s1g_oper , 
struct cfg80211_chan_def * chandef , bool tracking ) 
{ 
struct cfg80211_chan_def vht_chandef ; 
struct ieee80211_sta_ht_cap sta_ht_cap ; 
ieee80211_conn_flags_t ret ; 
u32 ht_cfreq ; 

memset ( chandef , 0 , sizeof ( struct cfg80211_chan_def ) ) ; 
chandef -> chan = channel ; 
chandef -> width = nl80211_chan_width_20_noht ; 
chandef -> center_freq1 = channel -> center_freq ; 
chandef -> freq1_offset = channel -> freq_offset ; 

if ( channel -> band == nl80211_band_6ghz ) { 
if ( ! ieee80211_chandef_he_6ghz_oper ( sdata , he_oper , eht_oper , 
chandef ) ) { 
mlme_dbg ( sdata , 
"bad 6 ghz operation, disabling ht/vht/he/eht\n" ) ; 
ret = ieee80211_conn_disable_ht | 
ieee80211_conn_disable_vht | 
ieee80211_conn_disable_he | 
ieee80211_conn_disable_eht ; 
} else { 
ret = 0 ; 
} 
vht_chandef = * chandef ; 
goto out ; 
} else if ( sband -> band == nl80211_band_s1ghz ) { 
if ( ! ieee80211_chandef_s1g_oper ( s1g_oper , chandef ) ) { 
sdata_info ( sdata , 
"missing s1g operation element? trying operating == primary\n" ) ; 
chandef -> width = ieee80211_s1g_channel_width ( channel ) ; 
} 

ret = ieee80211_conn_disable_ht | ieee80211_conn_disable_40mhz | 
ieee80211_conn_disable_vht | 
ieee80211_conn_disable_80p80mhz | 
ieee80211_conn_disable_160mhz ; 
goto out ; 
} 

memcpy ( & sta_ht_cap , & sband -> ht_cap , sizeof ( sta_ht_cap ) ) ; 
ieee80211_apply_htcap_overrides ( sdata , & sta_ht_cap ) ; 

if ( ! ht_oper || ! sta_ht_cap . ht_supported ) { 
mlme_dbg ( sdata , "ht operation missing / ht not supported\n" ) ; 
ret = ieee80211_conn_disable_ht | 
ieee80211_conn_disable_vht | 
ieee80211_conn_disable_he | 
ieee80211_conn_disable_eht ; 
goto out ; 
} 

chandef -> width = nl80211_chan_width_20 ; 

ht_cfreq = ieee80211_channel_to_frequency ( ht_oper -> primary_chan , 
channel -> band ) ; 

if ( ! tracking && channel -> center_freq != ht_cfreq ) { 







sdata_info ( sdata , 
"wrong control channel: center-freq: %d ht-cfreq: %d ht->primary_chan: %d band: %d - disabling ht\n" , 
channel -> center_freq , ht_cfreq , 
ht_oper -> primary_chan , channel -> band ) ; 
ret = ieee80211_conn_disable_ht | 
ieee80211_conn_disable_vht | 
ieee80211_conn_disable_he | 
ieee80211_conn_disable_eht ; 
goto out ; 
} 


if ( sta_ht_cap . cap & ieee80211_ht_cap_sup_width_20_40 ) { 
ieee80211_chandef_ht_oper ( ht_oper , chandef ) ; 
} else { 
mlme_dbg ( sdata , "40 mhz not supported\n" ) ; 

ret = ieee80211_conn_disable_vht ; 

ret |= ieee80211_conn_disable_40mhz ; 
goto out ; 
} 

if ( ! vht_oper || ! sband -> vht_cap . vht_supported ) { 
mlme_dbg ( sdata , "vht operation missing / vht not supported\n" ) ; 
ret = ieee80211_conn_disable_vht ; 
goto out ; 
} 

vht_chandef = * chandef ; 
if ( ! ( conn_flags & ieee80211_conn_disable_he ) && 
he_oper && 
( le32_to_cpu ( he_oper -> he_oper_params ) & 
ieee80211_he_operation_vht_oper_info ) ) { 
struct ieee80211_vht_operation he_oper_vht_cap ; 





memcpy ( & he_oper_vht_cap , he_oper -> optional , 3 ) ; 
he_oper_vht_cap . basic_mcs_set = cpu_to_le16 ( 0 ) ; 

if ( ! ieee80211_chandef_vht_oper ( & sdata -> local -> hw , vht_cap_info , 
& he_oper_vht_cap , ht_oper , 
& vht_chandef ) ) { 
if ( ! ( conn_flags & ieee80211_conn_disable_he ) ) 
sdata_info ( sdata , 
"he ap vht information is invalid, disabling he\n" ) ; 
ret = ieee80211_conn_disable_he | ieee80211_conn_disable_eht ; 
goto out ; 
} 
} else if ( ! ieee80211_chandef_vht_oper ( & sdata -> local -> hw , 
vht_cap_info , 
vht_oper , ht_oper , 
& vht_chandef ) ) { 
if ( ! ( conn_flags & ieee80211_conn_disable_vht ) ) 
sdata_info ( sdata , 
"ap vht information is invalid, disabling vht\n" ) ; 
ret = ieee80211_conn_disable_vht ; 
goto out ; 
} 

if ( ! cfg80211_chandef_valid ( & vht_chandef ) ) { 
if ( ! ( conn_flags & ieee80211_conn_disable_vht ) ) 
sdata_info ( sdata , 
"ap vht information is invalid, disabling vht\n" ) ; 
ret = ieee80211_conn_disable_vht ; 
goto out ; 
} 

if ( cfg80211_chandef_identical ( chandef , & vht_chandef ) ) { 
ret = 0 ; 
goto out ; 
} 

if ( ! cfg80211_chandef_compatible ( chandef , & vht_chandef ) ) { 
if ( ! ( conn_flags & ieee80211_conn_disable_vht ) ) 
sdata_info ( sdata , 
"ap vht information doesn't match ht, disabling vht\n" ) ; 
ret = ieee80211_conn_disable_vht ; 
goto out ; 
} 

* chandef = vht_chandef ; 






if ( eht_oper && ( eht_oper -> params & ieee80211_eht_oper_info_present ) ) { 
struct cfg80211_chan_def eht_chandef = * chandef ; 

ieee80211_chandef_eht_oper ( eht_oper , 
eht_chandef . width == 
nl80211_chan_width_160 , 
false , & eht_chandef ) ; 

if ( ! cfg80211_chandef_valid ( & eht_chandef ) ) { 
if ( ! ( conn_flags & ieee80211_conn_disable_eht ) ) 
sdata_info ( sdata , 
"ap eht information is invalid, disabling eht\n" ) ; 
ret = ieee80211_conn_disable_eht ; 
goto out ; 
} 

if ( ! cfg80211_chandef_compatible ( chandef , & eht_chandef ) ) { 
if ( ! ( conn_flags & ieee80211_conn_disable_eht ) ) 
sdata_info ( sdata , 
"ap eht information is incompatible, disabling eht\n" ) ; 
ret = ieee80211_conn_disable_eht ; 
goto out ; 
} 

* chandef = eht_chandef ; 
} 

ret = 0 ; 

out : 


















if ( tracking && 
cfg80211_chandef_identical ( chandef , & link -> conf -> chandef ) ) 
return ret ; 


if ( ret & ieee80211_conn_disable_vht ) 
vht_chandef = * chandef ; 









while ( ! cfg80211_chandef_usable ( sdata -> local -> hw . wiphy , chandef , 
tracking ? 0 : 
ieee80211_chan_disabled ) ) { 
if ( warn_on ( chandef -> width == nl80211_chan_width_20_noht ) ) { 
ret = ieee80211_conn_disable_ht | 
ieee80211_conn_disable_vht | 
ieee80211_conn_disable_he | 
ieee80211_conn_disable_eht ; 
break ; 
} 

ret |= ieee80211_chandef_downgrade ( chandef ) ; 
} 

if ( ! he_oper || ! cfg80211_chandef_usable ( sdata -> wdev . wiphy , chandef , 
ieee80211_chan_no_he ) ) 
ret |= ieee80211_conn_disable_he | ieee80211_conn_disable_eht ; 

if ( ! eht_oper || ! cfg80211_chandef_usable ( sdata -> wdev . wiphy , chandef , 
ieee80211_chan_no_eht ) ) 
ret |= ieee80211_conn_disable_eht ; 

if ( chandef -> width != vht_chandef . width && ! tracking ) 
sdata_info ( sdata , 
"capabilities/regulatory prevented using ap ht/vht configuration, downgraded\n" ) ; 

warn_on_once ( ! cfg80211_chandef_valid ( chandef ) ) ; 
return ret ; 
} 

static int ieee80211_config_bw ( struct ieee80211_link_data * link , 
const struct ieee80211_ht_cap * ht_cap , 
const struct ieee80211_vht_cap * vht_cap , 
const struct ieee80211_ht_operation * ht_oper , 
const struct ieee80211_vht_operation * vht_oper , 
const struct ieee80211_he_operation * he_oper , 
const struct ieee80211_eht_operation * eht_oper , 
const struct ieee80211_s1g_oper_ie * s1g_oper , 
const u8 * bssid , u64 * changed ) 
{ 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct ieee80211_channel * chan = link -> conf -> chandef . chan ; 
struct ieee80211_supported_band * sband = 
local -> hw . wiphy -> bands [ chan -> band ] ; 
struct cfg80211_chan_def chandef ; 
u16 ht_opmode ; 
ieee80211_conn_flags_t flags ; 
u32 vht_cap_info = 0 ; 
int ret ; 


if ( link -> u . mgd . conn_flags & ieee80211_conn_disable_ht || ! ht_oper ) 
return 0 ; 


if ( link -> u . mgd . conn_flags & ieee80211_conn_disable_vht ) 
vht_oper = null ; 


if ( link -> u . mgd . conn_flags & ieee80211_conn_disable_he || 
! ieee80211_get_he_iftype_cap ( sband , 
ieee80211_vif_type_p2p ( & sdata -> vif ) ) ) { 
he_oper = null ; 
eht_oper = null ; 
} 


if ( link -> u . mgd . conn_flags & ieee80211_conn_disable_eht || 
! ieee80211_get_eht_iftype_cap ( sband , 
ieee80211_vif_type_p2p ( & sdata -> vif ) ) ) 
eht_oper = null ; 





ht_opmode = le16_to_cpu ( ht_oper -> operation_mode ) ; 
if ( link -> conf -> ht_operation_mode != ht_opmode ) { 
* changed |= bss_changed_ht ; 
link -> conf -> ht_operation_mode = ht_opmode ; 
} 

if ( vht_cap ) 
vht_cap_info = le32_to_cpu ( vht_cap -> vht_cap_info ) ; 


flags = ieee80211_determine_chantype ( sdata , link , 
link -> u . mgd . conn_flags , 
sband , chan , vht_cap_info , 
ht_oper , vht_oper , 
he_oper , eht_oper , 
s1g_oper , & chandef , true ) ; 








if ( link -> u . mgd . conn_flags & ieee80211_conn_disable_80p80mhz && 
chandef . width == nl80211_chan_width_80p80 ) 
flags |= ieee80211_chandef_downgrade ( & chandef ) ; 
if ( link -> u . mgd . conn_flags & ieee80211_conn_disable_160mhz && 
chandef . width == nl80211_chan_width_160 ) 
flags |= ieee80211_chandef_downgrade ( & chandef ) ; 
if ( link -> u . mgd . conn_flags & ieee80211_conn_disable_40mhz && 
chandef . width > nl80211_chan_width_20 ) 
flags |= ieee80211_chandef_downgrade ( & chandef ) ; 

if ( cfg80211_chandef_identical ( & chandef , & link -> conf -> chandef ) ) 
return 0 ; 

link_info ( link , 
"ap %pm changed bandwidth, new config is %d.%03d mhz, width %d (%d.%03d/%d mhz)\n" , 
link -> u . mgd . bssid , chandef . chan -> center_freq , 
chandef . chan -> freq_offset , chandef . width , 
chandef . center_freq1 , chandef . freq1_offset , 
chandef . center_freq2 ) ; 

if ( flags != ( link -> u . mgd . conn_flags & 
( ieee80211_conn_disable_ht | 
ieee80211_conn_disable_vht | 
ieee80211_conn_disable_he | 
ieee80211_conn_disable_eht | 
ieee80211_conn_disable_40mhz | 
ieee80211_conn_disable_80p80mhz | 
ieee80211_conn_disable_160mhz | 
ieee80211_conn_disable_320mhz ) ) || 
! cfg80211_chandef_valid ( & chandef ) ) { 
sdata_info ( sdata , 
"ap %pm changed caps/bw in a way we can't support (0x%x/0x%x) - disconnect\n" , 
link -> u . mgd . bssid , flags , ifmgd -> flags ) ; 
return - einval ; 
} 

ret = ieee80211_link_change_bandwidth ( link , & chandef , changed ) ; 

if ( ret ) { 
sdata_info ( sdata , 
"ap %pm changed bandwidth to incompatible one - disconnect\n" , 
link -> u . mgd . bssid ) ; 
return ret ; 
} 

return 0 ; 
} 



static void ieee80211_add_ht_ie ( struct ieee80211_sub_if_data * sdata , 
struct sk_buff * skb , u8 ap_ht_param , 
struct ieee80211_supported_band * sband , 
struct ieee80211_channel * channel , 
enum ieee80211_smps_mode smps , 
ieee80211_conn_flags_t conn_flags ) 
{ 
u8 * pos ; 
u32 flags = channel -> flags ; 
u16 cap ; 
struct ieee80211_sta_ht_cap ht_cap ; 

build_bug_on ( sizeof ( ht_cap ) != sizeof ( sband -> ht_cap ) ) ; 

memcpy ( & ht_cap , & sband -> ht_cap , sizeof ( ht_cap ) ) ; 
ieee80211_apply_htcap_overrides ( sdata , & ht_cap ) ; 


cap = ht_cap . cap ; 

switch ( ap_ht_param & ieee80211_ht_param_cha_sec_offset ) { 
case ieee80211_ht_param_cha_sec_above : 
if ( flags & ieee80211_chan_no_ht40plus ) { 
cap &= ~ ieee80211_ht_cap_sup_width_20_40 ; 
cap &= ~ ieee80211_ht_cap_sgi_40 ; 
} 
break ; 
case ieee80211_ht_param_cha_sec_below : 
if ( flags & ieee80211_chan_no_ht40minus ) { 
cap &= ~ ieee80211_ht_cap_sup_width_20_40 ; 
cap &= ~ ieee80211_ht_cap_sgi_40 ; 
} 
break ; 
} 






if ( conn_flags & ieee80211_conn_disable_40mhz ) { 
cap &= ~ ieee80211_ht_cap_sup_width_20_40 ; 
cap &= ~ ieee80211_ht_cap_sgi_40 ; 
} 


cap &= ~ ieee80211_ht_cap_sm_ps ; 
switch ( smps ) { 
case ieee80211_smps_automatic : 
case ieee80211_smps_num_modes : 
warn_on ( 1 ) ; 
fallthrough ; 
case ieee80211_smps_off : 
cap |= wlan_ht_cap_sm_ps_disabled << 
ieee80211_ht_cap_sm_ps_shift ; 
break ; 
case ieee80211_smps_static : 
cap |= wlan_ht_cap_sm_ps_static << 
ieee80211_ht_cap_sm_ps_shift ; 
break ; 
case ieee80211_smps_dynamic : 
cap |= wlan_ht_cap_sm_ps_dynamic << 
ieee80211_ht_cap_sm_ps_shift ; 
break ; 
} 


pos = skb_put ( skb , sizeof ( struct ieee80211_ht_cap ) + 2 ) ; 
ieee80211_ie_build_ht_cap ( pos , & ht_cap , cap ) ; 
} 





static bool ieee80211_add_vht_ie ( struct ieee80211_sub_if_data * sdata , 
struct sk_buff * skb , 
struct ieee80211_supported_band * sband , 
struct ieee80211_vht_cap * ap_vht_cap , 
ieee80211_conn_flags_t conn_flags ) 
{ 
struct ieee80211_local * local = sdata -> local ; 
u8 * pos ; 
u32 cap ; 
struct ieee80211_sta_vht_cap vht_cap ; 
u32 mask , ap_bf_sts , our_bf_sts ; 
bool mu_mimo_owner = false ; 

build_bug_on ( sizeof ( vht_cap ) != sizeof ( sband -> vht_cap ) ) ; 

memcpy ( & vht_cap , & sband -> vht_cap , sizeof ( vht_cap ) ) ; 
ieee80211_apply_vhtcap_overrides ( sdata , & vht_cap ) ; 


cap = vht_cap . cap ; 

if ( conn_flags & ieee80211_conn_disable_80p80mhz ) { 
u32 bw = cap & ieee80211_vht_cap_supp_chan_width_mask ; 

cap &= ~ ieee80211_vht_cap_supp_chan_width_mask ; 
if ( bw == ieee80211_vht_cap_supp_chan_width_160mhz || 
bw == ieee80211_vht_cap_supp_chan_width_160_80plus80mhz ) 
cap |= ieee80211_vht_cap_supp_chan_width_160mhz ; 
} 

if ( conn_flags & ieee80211_conn_disable_160mhz ) { 
cap &= ~ ieee80211_vht_cap_short_gi_160 ; 
cap &= ~ ieee80211_vht_cap_supp_chan_width_mask ; 
} 





if ( ! ( ap_vht_cap -> vht_cap_info & 
cpu_to_le32 ( ieee80211_vht_cap_su_beamformer_capable ) ) ) 
cap &= ~ ( ieee80211_vht_cap_su_beamformee_capable | 
ieee80211_vht_cap_mu_beamformee_capable ) ; 
else if ( ! ( ap_vht_cap -> vht_cap_info & 
cpu_to_le32 ( ieee80211_vht_cap_mu_beamformer_capable ) ) ) 
cap &= ~ ieee80211_vht_cap_mu_beamformee_capable ; 








if ( cap & ieee80211_vht_cap_mu_beamformee_capable ) { 
bool disable_mu_mimo = false ; 
struct ieee80211_sub_if_data * other ; 

list_for_each_entry_rcu ( other , & local -> interfaces , list ) { 
if ( other -> vif . bss_conf . mu_mimo_owner ) { 
disable_mu_mimo = true ; 
break ; 
} 
} 
if ( disable_mu_mimo ) 
cap &= ~ ieee80211_vht_cap_mu_beamformee_capable ; 
else 
mu_mimo_owner = true ; 
} 

mask = ieee80211_vht_cap_beamformee_sts_mask ; 

ap_bf_sts = le32_to_cpu ( ap_vht_cap -> vht_cap_info ) & mask ; 
our_bf_sts = cap & mask ; 

if ( ap_bf_sts < our_bf_sts ) { 
cap &= ~ mask ; 
cap |= ap_bf_sts ; 
} 


pos = skb_put ( skb , sizeof ( struct ieee80211_vht_cap ) + 2 ) ; 
ieee80211_ie_build_vht_cap ( pos , & vht_cap , cap ) ; 

return mu_mimo_owner ; 
} 




static void ieee80211_add_he_ie ( struct ieee80211_sub_if_data * sdata , 
struct sk_buff * skb , 
struct ieee80211_supported_band * sband , 
enum ieee80211_smps_mode smps_mode , 
ieee80211_conn_flags_t conn_flags ) 
{ 
u8 * pos , * pre_he_pos ; 
const struct ieee80211_sta_he_cap * he_cap ; 
u8 he_cap_size ; 

he_cap = ieee80211_get_he_iftype_cap ( sband , 
ieee80211_vif_type_p2p ( & sdata -> vif ) ) ; 
if ( warn_on ( ! he_cap ) ) 
return ; 


he_cap_size = 
2 + 1 + sizeof ( he_cap -> he_cap_elem ) + 
ieee80211_he_mcs_nss_size ( & he_cap -> he_cap_elem ) + 
ieee80211_he_ppe_size ( he_cap -> ppe_thres [ 0 ] , 
he_cap -> he_cap_elem . phy_cap_info ) ; 
pos = skb_put ( skb , he_cap_size ) ; 
pre_he_pos = pos ; 
pos = ieee80211_ie_build_he_cap ( conn_flags , 
pos , he_cap , pos + he_cap_size ) ; 

skb_trim ( skb , skb -> len - ( pre_he_pos + he_cap_size - pos ) ) ; 

ieee80211_ie_build_he_6ghz_cap ( sdata , smps_mode , skb ) ; 
} 

static void ieee80211_add_eht_ie ( struct ieee80211_sub_if_data * sdata , 
struct sk_buff * skb , 
struct ieee80211_supported_band * sband ) 
{ 
u8 * pos ; 
const struct ieee80211_sta_he_cap * he_cap ; 
const struct ieee80211_sta_eht_cap * eht_cap ; 
u8 eht_cap_size ; 

he_cap = ieee80211_get_he_iftype_cap ( sband , 
ieee80211_vif_type_p2p ( & sdata -> vif ) ) ; 
eht_cap = ieee80211_get_eht_iftype_cap ( sband , 
ieee80211_vif_type_p2p ( & sdata -> vif ) ) ; 





if ( warn_on ( ! he_cap || ! eht_cap ) ) 
return ; 

eht_cap_size = 
2 + 1 + sizeof ( eht_cap -> eht_cap_elem ) + 
ieee80211_eht_mcs_nss_size ( & he_cap -> he_cap_elem , 
& eht_cap -> eht_cap_elem , 
false ) + 
ieee80211_eht_ppe_size ( eht_cap -> eht_ppe_thres [ 0 ] , 
eht_cap -> eht_cap_elem . phy_cap_info ) ; 
pos = skb_put ( skb , eht_cap_size ) ; 
ieee80211_ie_build_eht_cap ( pos , he_cap , eht_cap , pos + eht_cap_size , 
false ) ; 
} 

static void ieee80211_assoc_add_rates ( struct sk_buff * skb , 
enum nl80211_chan_width width , 
struct ieee80211_supported_band * sband , 
struct ieee80211_mgd_assoc_data * assoc_data ) 
{ 
unsigned int shift = ieee80211_chanwidth_get_shift ( width ) ; 
unsigned int rates_len , supp_rates_len ; 
u32 rates = 0 ; 
int i , count ; 
u8 * pos ; 

if ( assoc_data -> supp_rates_len ) { 






rates_len = ieee80211_parse_bitrates ( width , sband , 
assoc_data -> supp_rates , 
assoc_data -> supp_rates_len , 
& rates ) ; 
} else { 





rates_len = sband -> n_bitrates ; 
for ( i = 0 ; i < sband -> n_bitrates ; i ++ ) 
rates |= bit ( i ) ; 
} 

supp_rates_len = rates_len ; 
if ( supp_rates_len > 8 ) 
supp_rates_len = 8 ; 

pos = skb_put ( skb , supp_rates_len + 2 ) ; 
* pos ++ = wlan_eid_supp_rates ; 
* pos ++ = supp_rates_len ; 

count = 0 ; 
for ( i = 0 ; i < sband -> n_bitrates ; i ++ ) { 
if ( bit ( i ) & rates ) { 
int rate = div_round_up ( sband -> bitrates [ i ] . bitrate , 
5 * ( 1 << shift ) ) ; 
* pos ++ = ( u8 ) rate ; 
if ( ++ count == 8 ) 
break ; 
} 
} 

if ( rates_len > count ) { 
pos = skb_put ( skb , rates_len - count + 2 ) ; 
* pos ++ = wlan_eid_ext_supp_rates ; 
* pos ++ = rates_len - count ; 

for ( i ++ ; i < sband -> n_bitrates ; i ++ ) { 
if ( bit ( i ) & rates ) { 
int rate ; 

rate = div_round_up ( sband -> bitrates [ i ] . bitrate , 
5 * ( 1 << shift ) ) ; 
* pos ++ = ( u8 ) rate ; 
} 
} 
} 
} 

static size_t ieee80211_add_before_ht_elems ( struct sk_buff * skb , 
const u8 * elems , 
size_t elems_len , 
size_t offset ) 
{ 
size_t noffset ; 

static const u8 before_ht [ ] = { 
wlan_eid_ssid , 
wlan_eid_supp_rates , 
wlan_eid_ext_supp_rates , 
wlan_eid_pwr_capability , 
wlan_eid_supported_channels , 
wlan_eid_rsn , 
wlan_eid_qos_capa , 
wlan_eid_rrm_enabled_capabilities , 
wlan_eid_mobility_domain , 
wlan_eid_fast_bss_transition , 
wlan_eid_ric_data , 
wlan_eid_supported_regulatory_classes , 
} ; 
static const u8 after_ric [ ] = { 
wlan_eid_supported_regulatory_classes , 
wlan_eid_ht_capability , 
wlan_eid_bss_coex_2040 , 

wlan_eid_ext_capability , 
wlan_eid_qos_traffic_capa , 
wlan_eid_tim_bcast_req , 
wlan_eid_interworking , 

wlan_eid_vht_capability , 
wlan_eid_opmode_notif , 
} ; 

if ( ! elems_len ) 
return offset ; 

noffset = ieee80211_ie_split_ric ( elems , elems_len , 
before_ht , 
array_size ( before_ht ) , 
after_ric , 
array_size ( after_ric ) , 
offset ) ; 
skb_put_data ( skb , elems + offset , noffset - offset ) ; 

return noffset ; 
} 

static size_t ieee80211_add_before_vht_elems ( struct sk_buff * skb , 
const u8 * elems , 
size_t elems_len , 
size_t offset ) 
{ 
static const u8 before_vht [ ] = { 




wlan_eid_bss_coex_2040 , 
wlan_eid_ext_capability , 
wlan_eid_qos_traffic_capa , 
wlan_eid_tim_bcast_req , 
wlan_eid_interworking , 

} ; 
size_t noffset ; 

if ( ! elems_len ) 
return offset ; 


noffset = ieee80211_ie_split ( elems , elems_len , 
before_vht , array_size ( before_vht ) , 
offset ) ; 
skb_put_data ( skb , elems + offset , noffset - offset ) ; 

return noffset ; 
} 

static size_t ieee80211_add_before_he_elems ( struct sk_buff * skb , 
const u8 * elems , 
size_t elems_len , 
size_t offset ) 
{ 
static const u8 before_he [ ] = { 




wlan_eid_opmode_notif , 
wlan_eid_extension , wlan_eid_ext_future_chan_guidance , 

wlan_eid_extension , wlan_eid_ext_fils_session , 
wlan_eid_extension , wlan_eid_ext_fils_public_key , 
wlan_eid_extension , wlan_eid_ext_fils_key_confirm , 
wlan_eid_extension , wlan_eid_ext_fils_hlp_container , 
wlan_eid_extension , wlan_eid_ext_fils_ip_addr_assign , 

} ; 
size_t noffset ; 

if ( ! elems_len ) 
return offset ; 


noffset = ieee80211_ie_split ( elems , elems_len , 
before_he , array_size ( before_he ) , 
offset ) ; 
skb_put_data ( skb , elems + offset , noffset - offset ) ; 

return noffset ; 
} 




static void ieee80211_assoc_add_ml_elem ( struct ieee80211_sub_if_data * sdata , 
struct sk_buff * skb , u16 capab , 
const struct element * ext_capa , 
const u16 * present_elems ) ; 

static size_t ieee80211_assoc_link_elems ( struct ieee80211_sub_if_data * sdata , 
struct sk_buff * skb , u16 * capab , 
const struct element * ext_capa , 
const u8 * extra_elems , 
size_t extra_elems_len , 
unsigned int link_id , 
struct ieee80211_link_data * link , 
u16 * present_elems ) 
{ 
enum nl80211_iftype iftype = ieee80211_vif_type_p2p ( & sdata -> vif ) ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct ieee80211_mgd_assoc_data * assoc_data = ifmgd -> assoc_data ; 
struct cfg80211_bss * cbss = assoc_data -> link [ link_id ] . bss ; 
struct ieee80211_channel * chan = cbss -> channel ; 
const struct ieee80211_sband_iftype_data * iftd ; 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_supported_band * sband ; 
enum nl80211_chan_width width = nl80211_chan_width_20 ; 
struct ieee80211_chanctx_conf * chanctx_conf ; 
enum ieee80211_smps_mode smps_mode ; 
u16 orig_capab = * capab ; 
size_t offset = 0 ; 
int present_elems_len = 0 ; 
u8 * pos ; 
int i ; 








if ( link ) 
smps_mode = link -> smps_mode ; 
else if ( sdata -> u . mgd . powersave ) 
smps_mode = ieee80211_smps_dynamic ; 
else 
smps_mode = ieee80211_smps_off ; 

if ( link ) { 





rcu_read_lock ( ) ; 
chanctx_conf = rcu_dereference ( link -> conf -> chanctx_conf ) ; 
if ( chanctx_conf ) 
width = chanctx_conf -> def . width ; 
rcu_read_unlock ( ) ; 
} 

sband = local -> hw . wiphy -> bands [ chan -> band ] ; 
iftd = ieee80211_get_sband_iftype_data ( sband , iftype ) ; 

if ( sband -> band == nl80211_band_2ghz ) { 
* capab |= wlan_capability_short_slot_time ; 
* capab |= wlan_capability_short_preamble ; 
} 

if ( ( cbss -> capability & wlan_capability_spectrum_mgmt ) && 
ieee80211_hw_check ( & local -> hw , spectrum_mgmt ) ) 
* capab |= wlan_capability_spectrum_mgmt ; 

if ( sband -> band != nl80211_band_s1ghz ) 
ieee80211_assoc_add_rates ( skb , width , sband , assoc_data ) ; 

if ( * capab & wlan_capability_spectrum_mgmt || 
* capab & wlan_capability_radio_measure ) { 
struct cfg80211_chan_def chandef = { 
. width = width , 
. chan = chan , 
} ; 

pos = skb_put ( skb , 4 ) ; 
* pos ++ = wlan_eid_pwr_capability ; 
* pos ++ = 2 ; 
* pos ++ = 0 ; 

* pos ++ = ieee80211_chandef_max_power ( & chandef ) ; 
add_present_elem ( wlan_eid_pwr_capability ) ; 
} 






if ( * capab & wlan_capability_spectrum_mgmt && 
( sband -> band != nl80211_band_6ghz || 
! ext_capa || ext_capa -> datalen < 1 || 
! ( ext_capa -> data [ 0 ] & wlan_ext_capa1_ext_channel_switching ) ) ) { 

pos = skb_put ( skb , 2 * sband -> n_channels + 2 ) ; 
* pos ++ = wlan_eid_supported_channels ; 
* pos ++ = 2 * sband -> n_channels ; 
for ( i = 0 ; i < sband -> n_channels ; i ++ ) { 
int cf = sband -> channels [ i ] . center_freq ; 

* pos ++ = ieee80211_frequency_to_channel ( cf ) ; 
* pos ++ = 1 ; 
} 
add_present_elem ( wlan_eid_supported_channels ) ; 
} 


offset = ieee80211_add_before_ht_elems ( skb , extra_elems , 
extra_elems_len , 
offset ) ; 

if ( sband -> band != nl80211_band_6ghz && 
! ( assoc_data -> link [ link_id ] . conn_flags & ieee80211_conn_disable_ht ) ) { 
ieee80211_add_ht_ie ( sdata , skb , 
assoc_data -> link [ link_id ] . ap_ht_param , 
sband , chan , smps_mode , 
assoc_data -> link [ link_id ] . conn_flags ) ; 
add_present_elem ( wlan_eid_ht_capability ) ; 
} 


offset = ieee80211_add_before_vht_elems ( skb , extra_elems , 
extra_elems_len , 
offset ) ; 

if ( sband -> band != nl80211_band_6ghz && 
! ( assoc_data -> link [ link_id ] . conn_flags & ieee80211_conn_disable_vht ) ) { 
bool mu_mimo_owner = 
ieee80211_add_vht_ie ( sdata , skb , sband , 
& assoc_data -> link [ link_id ] . ap_vht_cap , 
assoc_data -> link [ link_id ] . conn_flags ) ; 

if ( link ) 
link -> conf -> mu_mimo_owner = mu_mimo_owner ; 
add_present_elem ( wlan_eid_vht_capability ) ; 
} 





if ( assoc_data -> link [ link_id ] . conn_flags & ieee80211_conn_disable_ht || 
( sband -> band == nl80211_band_5ghz && 
assoc_data -> link [ link_id ] . conn_flags & ieee80211_conn_disable_vht ) ) 
assoc_data -> link [ link_id ] . conn_flags |= 
ieee80211_conn_disable_he | 
ieee80211_conn_disable_eht ; 


offset = ieee80211_add_before_he_elems ( skb , extra_elems , 
extra_elems_len , 
offset ) ; 

if ( ! ( assoc_data -> link [ link_id ] . conn_flags & ieee80211_conn_disable_he ) ) { 
ieee80211_add_he_ie ( sdata , skb , sband , smps_mode , 
assoc_data -> link [ link_id ] . conn_flags ) ; 
add_present_ext_elem ( wlan_eid_ext_he_capability ) ; 
} 






if ( ! ( assoc_data -> link [ link_id ] . conn_flags & ieee80211_conn_disable_eht ) ) 
add_present_ext_elem ( wlan_eid_ext_eht_capability ) ; 

if ( link_id == assoc_data -> assoc_link_id ) 
ieee80211_assoc_add_ml_elem ( sdata , skb , orig_capab , ext_capa , 
present_elems ) ; 


present_elems = null ; 

if ( ! ( assoc_data -> link [ link_id ] . conn_flags & ieee80211_conn_disable_eht ) ) 
ieee80211_add_eht_ie ( sdata , skb , sband ) ; 

if ( sband -> band == nl80211_band_s1ghz ) { 
ieee80211_add_aid_request_ie ( sdata , skb ) ; 
ieee80211_add_s1g_capab_ie ( sdata , & sband -> s1g_cap , skb ) ; 
} 

if ( iftd && iftd -> vendor_elems . data && iftd -> vendor_elems . len ) 
skb_put_data ( skb , iftd -> vendor_elems . data , iftd -> vendor_elems . len ) ; 

if ( link ) 
link -> u . mgd . conn_flags = assoc_data -> link [ link_id ] . conn_flags ; 

return offset ; 
} 

static void ieee80211_add_non_inheritance_elem ( struct sk_buff * skb , 
const u16 * outer , 
const u16 * inner ) 
{ 
unsigned int skb_len = skb -> len ; 
bool added = false ; 
int i , j ; 
u8 * len , * list_len = null ; 

skb_put_u8 ( skb , wlan_eid_extension ) ; 
len = skb_put ( skb , 1 ) ; 
skb_put_u8 ( skb , wlan_eid_ext_non_inheritance ) ; 

for ( i = 0 ; i < present_elems_max && outer [ i ] ; i ++ ) { 
u16 elem = outer [ i ] ; 
bool have_inner = false ; 
bool at_extension = false ; 


warn_on ( at_extension && elem < present_elem_ext_offs ) ; 


if ( ! at_extension && elem >= present_elem_ext_offs ) { 
at_extension = true ; 
if ( ! list_len ) 
skb_put_u8 ( skb , 0 ) ; 
list_len = null ; 
} 

for ( j = 0 ; j < present_elems_max && inner [ j ] ; j ++ ) { 
if ( elem == inner [ j ] ) { 
have_inner = true ; 
break ; 
} 
} 

if ( have_inner ) 
continue ; 

if ( ! list_len ) { 
list_len = skb_put ( skb , 1 ) ; 
* list_len = 0 ; 
} 
* list_len += 1 ; 
skb_put_u8 ( skb , ( u8 ) elem ) ; 
} 

if ( ! added ) 
skb_trim ( skb , skb_len ) ; 
else 
* len = skb -> len - skb_len - 2 ; 
} 

static void ieee80211_assoc_add_ml_elem ( struct ieee80211_sub_if_data * sdata , 
struct sk_buff * skb , u16 capab , 
const struct element * ext_capa , 
const u16 * outer_present_elems ) 
{ 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct ieee80211_mgd_assoc_data * assoc_data = ifmgd -> assoc_data ; 
struct ieee80211_multi_link_elem * ml_elem ; 
struct ieee80211_mle_basic_common_info * common ; 
const struct wiphy_iftype_ext_capab * ift_ext_capa ; 
__le16 eml_capa = 0 , mld_capa_ops = 0 ; 
unsigned int link_id ; 
u8 * ml_elem_len ; 
void * capab_pos ; 

if ( ! sdata -> vif . valid_links ) 
return ; 

ift_ext_capa = cfg80211_get_iftype_ext_capa ( local -> hw . wiphy , 
ieee80211_vif_type_p2p ( & sdata -> vif ) ) ; 
if ( ift_ext_capa ) { 
eml_capa = cpu_to_le16 ( ift_ext_capa -> eml_capabilities ) ; 
mld_capa_ops = cpu_to_le16 ( ift_ext_capa -> mld_capa_and_ops ) ; 
} 

skb_put_u8 ( skb , wlan_eid_extension ) ; 
ml_elem_len = skb_put ( skb , 1 ) ; 
skb_put_u8 ( skb , wlan_eid_ext_eht_multi_link ) ; 
ml_elem = skb_put ( skb , sizeof ( * ml_elem ) ) ; 
ml_elem -> control = 
cpu_to_le16 ( ieee80211_ml_control_type_basic | 
ieee80211_mlc_basic_pres_mld_capa_op ) ; 
common = skb_put ( skb , sizeof ( * common ) ) ; 
common -> len = sizeof ( * common ) + 
2 ; 
memcpy ( common -> mld_mac_addr , sdata -> vif . addr , eth_alen ) ; 


if ( eml_capa & 
cpu_to_le16 ( ( ieee80211_eml_cap_emlsr_supp | 
ieee80211_eml_cap_emlmr_support ) ) ) { 
common -> len += 2 ; 
ml_elem -> control |= 
cpu_to_le16 ( ieee80211_mlc_basic_pres_eml_capa ) ; 
skb_put_data ( skb , & eml_capa , sizeof ( eml_capa ) ) ; 
} 

mld_capa_ops &= ~ cpu_to_le16 ( ieee80211_mld_cap_op_tid_to_link_map_neg_supp ) ; 
skb_put_data ( skb , & mld_capa_ops , sizeof ( mld_capa_ops ) ) ; 

for ( link_id = 0 ; link_id < ieee80211_mld_max_num_links ; link_id ++ ) { 
u16 link_present_elems [ present_elems_max ] = { } ; 
const u8 * extra_elems ; 
size_t extra_elems_len ; 
size_t extra_used ; 
u8 * subelem_len = null ; 
__le16 ctrl ; 

if ( ! assoc_data -> link [ link_id ] . bss || 
link_id == assoc_data -> assoc_link_id ) 
continue ; 

extra_elems = assoc_data -> link [ link_id ] . elems ; 
extra_elems_len = assoc_data -> link [ link_id ] . elems_len ; 

skb_put_u8 ( skb , ieee80211_mle_subelem_per_sta_profile ) ; 
subelem_len = skb_put ( skb , 1 ) ; 

ctrl = cpu_to_le16 ( link_id | 
ieee80211_mle_sta_control_complete_profile | 
ieee80211_mle_sta_control_sta_mac_addr_present ) ; 
skb_put_data ( skb , & ctrl , sizeof ( ctrl ) ) ; 
skb_put_u8 ( skb , 1 + eth_alen ) ; 
skb_put_data ( skb , assoc_data -> link [ link_id ] . addr , 
eth_alen ) ; 








capab_pos = skb_put ( skb , 2 ) ; 

extra_used = ieee80211_assoc_link_elems ( sdata , skb , & capab , 
ext_capa , 
extra_elems , 
extra_elems_len , 
link_id , null , 
link_present_elems ) ; 
if ( extra_elems ) 
skb_put_data ( skb , extra_elems + extra_used , 
extra_elems_len - extra_used ) ; 

put_unaligned_le16 ( capab , capab_pos ) ; 

ieee80211_add_non_inheritance_elem ( skb , outer_present_elems , 
link_present_elems ) ; 

ieee80211_fragment_element ( skb , subelem_len ) ; 
} 

ieee80211_fragment_element ( skb , ml_elem_len ) ; 
} 

static int ieee80211_send_assoc ( struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct ieee80211_mgd_assoc_data * assoc_data = ifmgd -> assoc_data ; 
struct ieee80211_link_data * link ; 
struct sk_buff * skb ; 
struct ieee80211_mgmt * mgmt ; 
u8 * pos , qos_info , * ie_start ; 
size_t offset , noffset ; 
u16 capab = wlan_capability_ess , link_capab ; 
__le16 listen_int ; 
struct element * ext_capa = null ; 
enum nl80211_iftype iftype = ieee80211_vif_type_p2p ( & sdata -> vif ) ; 
struct ieee80211_prep_tx_info info = { } ; 
unsigned int link_id , n_links = 0 ; 
u16 present_elems [ present_elems_max ] = { } ; 
void * capab_pos ; 
size_t size ; 
int ret ; 


if ( assoc_data -> ie_len ) 
ext_capa = ( void * ) cfg80211_find_elem ( wlan_eid_ext_capability , 
assoc_data -> ie , 
assoc_data -> ie_len ) ; 

sdata_assert_lock ( sdata ) ; 

size = local -> hw . extra_tx_headroom + 
sizeof ( * mgmt ) + 
2 + assoc_data -> ssid_len + 
assoc_data -> ie_len + 
( assoc_data -> fils_kek_len ? 16 : 0 ) + 
9 ; 

for ( link_id = 0 ; link_id < ieee80211_mld_max_num_links ; link_id ++ ) { 
struct cfg80211_bss * cbss = assoc_data -> link [ link_id ] . bss ; 
const struct ieee80211_sband_iftype_data * iftd ; 
struct ieee80211_supported_band * sband ; 

if ( ! cbss ) 
continue ; 

sband = local -> hw . wiphy -> bands [ cbss -> channel -> band ] ; 

n_links ++ ; 

size += assoc_data -> link [ link_id ] . elems_len ; 

size += 4 + sband -> n_bitrates ; 

size += 2 + 2 * sband -> n_channels ; 

iftd = ieee80211_get_sband_iftype_data ( sband , iftype ) ; 
if ( iftd ) 
size += iftd -> vendor_elems . len ; 


size += 4 ; 


size += 2 + sizeof ( struct ieee80211_ht_cap ) ; 
size += 2 + sizeof ( struct ieee80211_vht_cap ) ; 
size += 2 + 1 + sizeof ( struct ieee80211_he_cap_elem ) + 
sizeof ( struct ieee80211_he_mcs_nss_supp ) + 
ieee80211_he_ppe_thres_max_len ; 

if ( sband -> band == nl80211_band_6ghz ) 
size += 2 + 1 + sizeof ( struct ieee80211_he_6ghz_capa ) ; 

size += 2 + 1 + sizeof ( struct ieee80211_eht_cap_elem ) + 
sizeof ( struct ieee80211_eht_mcs_nss_supp ) + 
ieee80211_eht_ppe_thres_max_len ; 


size += 2 + 2 + present_elems_max ; 


if ( cbss -> capability & wlan_capability_privacy ) 
capab |= wlan_capability_privacy ; 
} 

if ( sdata -> vif . valid_links ) { 

size += sizeof ( struct ieee80211_multi_link_elem ) ; 

size += sizeof ( struct ieee80211_mle_basic_common_info ) + 
2 + 
2 ; 






size += ( n_links - 1 ) * 
( 1 + 1 + 
2 + 
1 + eth_alen + 2 ) ; 
} 

link = sdata_dereference ( sdata -> link [ assoc_data -> assoc_link_id ] , sdata ) ; 
if ( warn_on ( ! link ) ) 
return - einval ; 

if ( warn_on ( ! assoc_data -> link [ assoc_data -> assoc_link_id ] . bss ) ) 
return - einval ; 

skb = alloc_skb ( size , gfp_kernel ) ; 
if ( ! skb ) 
return - enomem ; 

skb_reserve ( skb , local -> hw . extra_tx_headroom ) ; 

if ( ifmgd -> flags & ieee80211_sta_enable_rrm ) 
capab |= wlan_capability_radio_measure ; 


if ( ieee80211_hw_check ( & local -> hw , supports_only_he_multi_bssid ) && 
! ( link -> u . mgd . conn_flags & ieee80211_conn_disable_he ) && 
ext_capa && ext_capa -> datalen >= 3 ) 
ext_capa -> data [ 2 ] |= wlan_ext_capa3_multi_bssid_support ; 

mgmt = skb_put_zero ( skb , 24 ) ; 
memcpy ( mgmt -> da , sdata -> vif . cfg . ap_addr , eth_alen ) ; 
memcpy ( mgmt -> sa , sdata -> vif . addr , eth_alen ) ; 
memcpy ( mgmt -> bssid , sdata -> vif . cfg . ap_addr , eth_alen ) ; 

listen_int = cpu_to_le16 ( assoc_data -> s1g ? 
ieee80211_encode_usf ( local -> hw . conf . listen_interval ) : 
local -> hw . conf . listen_interval ) ; 
if ( ! is_zero_ether_addr ( assoc_data -> prev_ap_addr ) ) { 
skb_put ( skb , 10 ) ; 
mgmt -> frame_control = cpu_to_le16 ( ieee80211_ftype_mgmt | 
ieee80211_stype_reassoc_req ) ; 
capab_pos = & mgmt -> u . reassoc_req . capab_info ; 
mgmt -> u . reassoc_req . listen_interval = listen_int ; 
memcpy ( mgmt -> u . reassoc_req . current_ap , 
assoc_data -> prev_ap_addr , eth_alen ) ; 
info . subtype = ieee80211_stype_reassoc_req ; 
} else { 
skb_put ( skb , 4 ) ; 
mgmt -> frame_control = cpu_to_le16 ( ieee80211_ftype_mgmt | 
ieee80211_stype_assoc_req ) ; 
capab_pos = & mgmt -> u . assoc_req . capab_info ; 
mgmt -> u . assoc_req . listen_interval = listen_int ; 
info . subtype = ieee80211_stype_assoc_req ; 
} 


pos = skb_put ( skb , 2 + assoc_data -> ssid_len ) ; 
ie_start = pos ; 
* pos ++ = wlan_eid_ssid ; 
* pos ++ = assoc_data -> ssid_len ; 
memcpy ( pos , assoc_data -> ssid , assoc_data -> ssid_len ) ; 


link_capab = capab ; 
offset = ieee80211_assoc_link_elems ( sdata , skb , & link_capab , 
ext_capa , 
assoc_data -> ie , 
assoc_data -> ie_len , 
assoc_data -> assoc_link_id , link , 
present_elems ) ; 
put_unaligned_le16 ( link_capab , capab_pos ) ; 


if ( assoc_data -> ie_len ) { 
noffset = ieee80211_ie_split_vendor ( assoc_data -> ie , 
assoc_data -> ie_len , 
offset ) ; 
skb_put_data ( skb , assoc_data -> ie + offset , noffset - offset ) ; 
offset = noffset ; 
} 

if ( assoc_data -> wmm ) { 
if ( assoc_data -> uapsd ) { 
qos_info = ifmgd -> uapsd_queues ; 
qos_info |= ( ifmgd -> uapsd_max_sp_len << 
ieee80211_wmm_ie_sta_qosinfo_sp_shift ) ; 
} else { 
qos_info = 0 ; 
} 

pos = ieee80211_add_wmm_info_ie ( skb_put ( skb , 9 ) , qos_info ) ; 
} 


if ( assoc_data -> ie_len ) { 
noffset = assoc_data -> ie_len ; 
skb_put_data ( skb , assoc_data -> ie + offset , noffset - offset ) ; 
} 

if ( assoc_data -> fils_kek_len ) { 
ret = fils_encrypt_assoc_req ( skb , assoc_data ) ; 
if ( ret < 0 ) { 
dev_kfree_skb ( skb ) ; 
return ret ; 
} 
} 

pos = skb_tail_pointer ( skb ) ; 
kfree ( ifmgd -> assoc_req_ies ) ; 
ifmgd -> assoc_req_ies = kmemdup ( ie_start , pos - ie_start , gfp_atomic ) ; 
if ( ! ifmgd -> assoc_req_ies ) { 
dev_kfree_skb ( skb ) ; 
return - enomem ; 
} 

ifmgd -> assoc_req_ies_len = pos - ie_start ; 

drv_mgd_prepare_tx ( local , sdata , & info ) ; 

ieee80211_skb_cb ( skb ) -> flags |= ieee80211_tx_intfl_dont_encrypt ; 
if ( ieee80211_hw_check ( & local -> hw , reports_tx_ack_status ) ) 
ieee80211_skb_cb ( skb ) -> flags |= ieee80211_tx_ctl_req_tx_status | 
ieee80211_tx_intfl_mlme_conn_tx ; 
ieee80211_tx_skb ( sdata , skb ) ; 

return 0 ; 
} 

void ieee80211_send_pspoll ( struct ieee80211_local * local , 
struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_pspoll * pspoll ; 
struct sk_buff * skb ; 

skb = ieee80211_pspoll_get ( & local -> hw , & sdata -> vif ) ; 
if ( ! skb ) 
return ; 

pspoll = ( struct ieee80211_pspoll * ) skb -> data ; 
pspoll -> frame_control |= cpu_to_le16 ( ieee80211_fctl_pm ) ; 

ieee80211_skb_cb ( skb ) -> flags |= ieee80211_tx_intfl_dont_encrypt ; 
ieee80211_tx_skb ( sdata , skb ) ; 
} 

void ieee80211_send_nullfunc ( struct ieee80211_local * local , 
struct ieee80211_sub_if_data * sdata , 
bool powersave ) 
{ 
struct sk_buff * skb ; 
struct ieee80211_hdr_3addr * nullfunc ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 

skb = ieee80211_nullfunc_get ( & local -> hw , & sdata -> vif , - 1 , 
! ieee80211_hw_check ( & local -> hw , 
doesnt_support_qos_ndp ) ) ; 
if ( ! skb ) 
return ; 

nullfunc = ( struct ieee80211_hdr_3addr * ) skb -> data ; 
if ( powersave ) 
nullfunc -> frame_control |= cpu_to_le16 ( ieee80211_fctl_pm ) ; 

ieee80211_skb_cb ( skb ) -> flags |= ieee80211_tx_intfl_dont_encrypt | 
ieee80211_tx_intfl_offchan_tx_ok ; 

if ( ieee80211_hw_check ( & local -> hw , reports_tx_ack_status ) ) 
ieee80211_skb_cb ( skb ) -> flags |= ieee80211_tx_ctl_req_tx_status ; 

if ( ifmgd -> flags & ieee80211_sta_connection_poll ) 
ieee80211_skb_cb ( skb ) -> flags |= ieee80211_tx_ctl_use_minrate ; 

ieee80211_tx_skb ( sdata , skb ) ; 
} 

void ieee80211_send_4addr_nullfunc ( struct ieee80211_local * local , 
struct ieee80211_sub_if_data * sdata ) 
{ 
struct sk_buff * skb ; 
struct ieee80211_hdr * nullfunc ; 
__le16 fc ; 

if ( warn_on ( sdata -> vif . type != nl80211_iftype_station ) ) 
return ; 

skb = dev_alloc_skb ( local -> hw . extra_tx_headroom + 30 ) ; 
if ( ! skb ) 
return ; 

skb_reserve ( skb , local -> hw . extra_tx_headroom ) ; 

nullfunc = skb_put_zero ( skb , 30 ) ; 
fc = cpu_to_le16 ( ieee80211_ftype_data | ieee80211_stype_nullfunc | 
ieee80211_fctl_fromds | ieee80211_fctl_tods ) ; 
nullfunc -> frame_control = fc ; 
memcpy ( nullfunc -> addr1 , sdata -> deflink . u . mgd . bssid , eth_alen ) ; 
memcpy ( nullfunc -> addr2 , sdata -> vif . addr , eth_alen ) ; 
memcpy ( nullfunc -> addr3 , sdata -> deflink . u . mgd . bssid , eth_alen ) ; 
memcpy ( nullfunc -> addr4 , sdata -> vif . addr , eth_alen ) ; 

ieee80211_skb_cb ( skb ) -> flags |= ieee80211_tx_intfl_dont_encrypt ; 
ieee80211_skb_cb ( skb ) -> flags |= ieee80211_tx_ctl_use_minrate ; 
ieee80211_tx_skb ( sdata , skb ) ; 
} 


static void ieee80211_chswitch_work ( struct work_struct * work ) 
{ 
struct ieee80211_link_data * link = 
container_of ( work , struct ieee80211_link_data , u . mgd . chswitch_work ) ; 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
int ret ; 

if ( ! ieee80211_sdata_running ( sdata ) ) 
return ; 

sdata_lock ( sdata ) ; 
mutex_lock ( & local -> mtx ) ; 
mutex_lock ( & local -> chanctx_mtx ) ; 

if ( ! ifmgd -> associated ) 
goto out ; 

if ( ! link -> conf -> csa_active ) 
goto out ; 








if ( link -> reserved_chanctx ) { 





if ( link -> reserved_ready ) 
goto out ; 

ret = ieee80211_link_use_reserved_context ( link ) ; 
if ( ret ) { 
sdata_info ( sdata , 
"failed to use reserved channel context, disconnecting (err=%d)\n" , 
ret ) ; 
ieee80211_queue_work ( & sdata -> local -> hw , 
& ifmgd -> csa_connection_drop_work ) ; 
goto out ; 
} 

goto out ; 
} 

if ( ! cfg80211_chandef_identical ( & link -> conf -> chandef , 
& link -> csa_chandef ) ) { 
sdata_info ( sdata , 
"failed to finalize channel switch, disconnecting\n" ) ; 
ieee80211_queue_work ( & sdata -> local -> hw , 
& ifmgd -> csa_connection_drop_work ) ; 
goto out ; 
} 

link -> u . mgd . csa_waiting_bcn = true ; 

ieee80211_sta_reset_beacon_monitor ( sdata ) ; 
ieee80211_sta_reset_conn_monitor ( sdata ) ; 

out : 
mutex_unlock ( & local -> chanctx_mtx ) ; 
mutex_unlock ( & local -> mtx ) ; 
sdata_unlock ( sdata ) ; 
} 

static void ieee80211_chswitch_post_beacon ( struct ieee80211_link_data * link ) 
{ 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
int ret ; 

sdata_assert_lock ( sdata ) ; 

warn_on ( ! link -> conf -> csa_active ) ; 

if ( link -> csa_block_tx ) { 
ieee80211_wake_vif_queues ( local , sdata , 
ieee80211_queue_stop_reason_csa ) ; 
link -> csa_block_tx = false ; 
} 

link -> conf -> csa_active = false ; 
link -> u . mgd . csa_waiting_bcn = false ; 




link -> u . mgd . beacon_crc_valid = false ; 

ret = drv_post_channel_switch ( sdata ) ; 
if ( ret ) { 
sdata_info ( sdata , 
"driver post channel switch failed, disconnecting\n" ) ; 
ieee80211_queue_work ( & local -> hw , 
& ifmgd -> csa_connection_drop_work ) ; 
return ; 
} 

cfg80211_ch_switch_notify ( sdata -> dev , & link -> reserved_chandef , 0 , 0 ) ; 
} 

void ieee80211_chswitch_done ( struct ieee80211_vif * vif , bool success ) 
{ 
struct ieee80211_sub_if_data * sdata = vif_to_sdata ( vif ) ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 

if ( warn_on ( sdata -> vif . valid_links ) ) 
success = false ; 

trace_api_chswitch_done ( sdata , success ) ; 
if ( ! success ) { 
sdata_info ( sdata , 
"driver channel switch failed, disconnecting\n" ) ; 
ieee80211_queue_work ( & sdata -> local -> hw , 
& ifmgd -> csa_connection_drop_work ) ; 
} else { 
ieee80211_queue_work ( & sdata -> local -> hw , 
& sdata -> deflink . u . mgd . chswitch_work ) ; 
} 
} 
export_symbol ( ieee80211_chswitch_done ) ; 

static void ieee80211_chswitch_timer ( struct timer_list * t ) 
{ 
struct ieee80211_link_data * link = 
from_timer ( link , t , u . mgd . chswitch_timer ) ; 

ieee80211_queue_work ( & link -> sdata -> local -> hw , 
& link -> u . mgd . chswitch_work ) ; 
} 

static void 
ieee80211_sta_abort_chanswitch ( struct ieee80211_link_data * link ) 
{ 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 
struct ieee80211_local * local = sdata -> local ; 

if ( ! local -> ops -> abort_channel_switch ) 
return ; 

mutex_lock ( & local -> mtx ) ; 

mutex_lock ( & local -> chanctx_mtx ) ; 
ieee80211_link_unreserve_chanctx ( link ) ; 
mutex_unlock ( & local -> chanctx_mtx ) ; 

if ( link -> csa_block_tx ) 
ieee80211_wake_vif_queues ( local , sdata , 
ieee80211_queue_stop_reason_csa ) ; 

link -> csa_block_tx = false ; 
link -> conf -> csa_active = false ; 

mutex_unlock ( & local -> mtx ) ; 

drv_abort_channel_switch ( sdata ) ; 
} 

static void 
ieee80211_sta_process_chanswitch ( struct ieee80211_link_data * link , 
u64 timestamp , u32 device_timestamp , 
struct ieee802_11_elems * elems , 
bool beacon ) 
{ 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct cfg80211_bss * cbss = link -> u . mgd . bss ; 
struct ieee80211_chanctx_conf * conf ; 
struct ieee80211_chanctx * chanctx ; 
enum nl80211_band current_band ; 
struct ieee80211_csa_ie csa_ie ; 
struct ieee80211_channel_switch ch_switch ; 
struct ieee80211_bss * bss ; 
int res ; 

sdata_assert_lock ( sdata ) ; 

if ( ! cbss ) 
return ; 

if ( local -> scanning ) 
return ; 

current_band = cbss -> channel -> band ; 
bss = ( void * ) cbss -> priv ; 
res = ieee80211_parse_ch_switch_ie ( sdata , elems , current_band , 
bss -> vht_cap_info , 
link -> u . mgd . conn_flags , 
link -> u . mgd . bssid , & csa_ie ) ; 

if ( ! res ) { 
ch_switch . timestamp = timestamp ; 
ch_switch . device_timestamp = device_timestamp ; 
ch_switch . block_tx = csa_ie . mode ; 
ch_switch . chandef = csa_ie . chandef ; 
ch_switch . count = csa_ie . count ; 
ch_switch . delay = csa_ie . max_switch_time ; 
} 

if ( res < 0 ) 
goto lock_and_drop_connection ; 

if ( beacon && link -> conf -> csa_active && 
! link -> u . mgd . csa_waiting_bcn ) { 
if ( res ) 
ieee80211_sta_abort_chanswitch ( link ) ; 
else 
drv_channel_switch_rx_beacon ( sdata , & ch_switch ) ; 
return ; 
} else if ( link -> conf -> csa_active || res ) { 

return ; 
} 

if ( link -> conf -> chandef . chan -> band != 
csa_ie . chandef . chan -> band ) { 
sdata_info ( sdata , 
"ap %pm switches to different band (%d mhz, width:%d, cf1/2: %d/%d mhz), disconnecting\n" , 
link -> u . mgd . bssid , 
csa_ie . chandef . chan -> center_freq , 
csa_ie . chandef . width , csa_ie . chandef . center_freq1 , 
csa_ie . chandef . center_freq2 ) ; 
goto lock_and_drop_connection ; 
} 

if ( ! cfg80211_chandef_usable ( local -> hw . wiphy , & csa_ie . chandef , 
ieee80211_chan_disabled ) ) { 
sdata_info ( sdata , 
"ap %pm switches to unsupported channel " 
"(%d.%03d mhz, width:%d, cf1/2: %d.%03d/%d mhz), " 
"disconnecting\n" , 
link -> u . mgd . bssid , 
csa_ie . chandef . chan -> center_freq , 
csa_ie . chandef . chan -> freq_offset , 
csa_ie . chandef . width , csa_ie . chandef . center_freq1 , 
csa_ie . chandef . freq1_offset , 
csa_ie . chandef . center_freq2 ) ; 
goto lock_and_drop_connection ; 
} 

if ( cfg80211_chandef_identical ( & csa_ie . chandef , 
& link -> conf -> chandef ) && 
( ! csa_ie . mode || ! beacon ) ) { 
if ( link -> u . mgd . csa_ignored_same_chan ) 
return ; 
sdata_info ( sdata , 
"ap %pm tries to chanswitch to same channel, ignore\n" , 
link -> u . mgd . bssid ) ; 
link -> u . mgd . csa_ignored_same_chan = true ; 
return ; 
} 







ieee80211_teardown_tdls_peers ( sdata ) ; 

mutex_lock ( & local -> mtx ) ; 
mutex_lock ( & local -> chanctx_mtx ) ; 
conf = rcu_dereference_protected ( link -> conf -> chanctx_conf , 
lockdep_is_held ( & local -> chanctx_mtx ) ) ; 
if ( ! conf ) { 
sdata_info ( sdata , 
"no channel context assigned to vif?, disconnecting\n" ) ; 
goto drop_connection ; 
} 

chanctx = container_of ( conf , struct ieee80211_chanctx , conf ) ; 

if ( local -> use_chanctx && 
! ieee80211_hw_check ( & local -> hw , chanctx_sta_csa ) ) { 
sdata_info ( sdata , 
"driver doesn't support chan-switch with channel contexts\n" ) ; 
goto drop_connection ; 
} 

if ( drv_pre_channel_switch ( sdata , & ch_switch ) ) { 
sdata_info ( sdata , 
"preparing for channel switch failed, disconnecting\n" ) ; 
goto drop_connection ; 
} 

res = ieee80211_link_reserve_chanctx ( link , & csa_ie . chandef , 
chanctx -> mode , false ) ; 
if ( res ) { 
sdata_info ( sdata , 
"failed to reserve channel context for channel switch, disconnecting (err=%d)\n" , 
res ) ; 
goto drop_connection ; 
} 
mutex_unlock ( & local -> chanctx_mtx ) ; 

link -> conf -> csa_active = true ; 
link -> csa_chandef = csa_ie . chandef ; 
link -> csa_block_tx = csa_ie . mode ; 
link -> u . mgd . csa_ignored_same_chan = false ; 
link -> u . mgd . beacon_crc_valid = false ; 

if ( link -> csa_block_tx ) 
ieee80211_stop_vif_queues ( local , sdata , 
ieee80211_queue_stop_reason_csa ) ; 
mutex_unlock ( & local -> mtx ) ; 

cfg80211_ch_switch_started_notify ( sdata -> dev , & csa_ie . chandef , 0 , 
csa_ie . count , csa_ie . mode , 0 ) ; 

if ( local -> ops -> channel_switch ) { 

drv_channel_switch ( local , sdata , & ch_switch ) ; 
return ; 
} 


if ( csa_ie . count <= 1 ) 
ieee80211_queue_work ( & local -> hw , & link -> u . mgd . chswitch_work ) ; 
else 
mod_timer ( & link -> u . mgd . chswitch_timer , 
tu_to_exp_time ( ( csa_ie . count - 1 ) * 
cbss -> beacon_interval ) ) ; 
return ; 
lock_and_drop_connection : 
mutex_lock ( & local -> mtx ) ; 
mutex_lock ( & local -> chanctx_mtx ) ; 
drop_connection : 







link -> conf -> csa_active = true ; 
link -> csa_block_tx = csa_ie . mode ; 

ieee80211_queue_work ( & local -> hw , & ifmgd -> csa_connection_drop_work ) ; 
mutex_unlock ( & local -> chanctx_mtx ) ; 
mutex_unlock ( & local -> mtx ) ; 
} 

static bool 
ieee80211_find_80211h_pwr_constr ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_channel * channel , 
const u8 * country_ie , u8 country_ie_len , 
const u8 * pwr_constr_elem , 
int * chan_pwr , int * pwr_reduction ) 
{ 
struct ieee80211_country_ie_triplet * triplet ; 
int chan = ieee80211_frequency_to_channel ( channel -> center_freq ) ; 
int i , chan_increment ; 
bool have_chan_pwr = false ; 


if ( country_ie_len % 2 || country_ie_len < ieee80211_country_ie_min_len ) 
return false ; 

triplet = ( void * ) ( country_ie + 3 ) ; 
country_ie_len -= 3 ; 

switch ( channel -> band ) { 
default : 
warn_on_once ( 1 ) ; 
fallthrough ; 
case nl80211_band_2ghz : 
case nl80211_band_60ghz : 
case nl80211_band_lc : 
chan_increment = 1 ; 
break ; 
case nl80211_band_5ghz : 
chan_increment = 4 ; 
break ; 
case nl80211_band_6ghz : 







return false ; 
} 


while ( country_ie_len >= 3 ) { 
u8 first_channel = triplet -> chans . first_channel ; 

if ( first_channel >= ieee80211_country_extension_id ) 
goto next ; 

for ( i = 0 ; i < triplet -> chans . num_channels ; i ++ ) { 
if ( first_channel + i * chan_increment == chan ) { 
have_chan_pwr = true ; 
* chan_pwr = triplet -> chans . max_power ; 
break ; 
} 
} 
if ( have_chan_pwr ) 
break ; 

next : 
triplet ++ ; 
country_ie_len -= 3 ; 
} 

if ( have_chan_pwr && pwr_constr_elem ) 
* pwr_reduction = * pwr_constr_elem ; 
else 
* pwr_reduction = 0 ; 

return have_chan_pwr ; 
} 

static void ieee80211_find_cisco_dtpc ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_channel * channel , 
const u8 * cisco_dtpc_ie , 
int * pwr_level ) 
{ 






* pwr_level = ( __s8 ) cisco_dtpc_ie [ 4 ] ; 
} 

static u32 ieee80211_handle_pwr_constr ( struct ieee80211_link_data * link , 
struct ieee80211_channel * channel , 
struct ieee80211_mgmt * mgmt , 
const u8 * country_ie , u8 country_ie_len , 
const u8 * pwr_constr_ie , 
const u8 * cisco_dtpc_ie ) 
{ 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 
bool has_80211h_pwr = false , has_cisco_pwr = false ; 
int chan_pwr = 0 , pwr_reduction_80211h = 0 ; 
int pwr_level_cisco , pwr_level_80211h ; 
int new_ap_level ; 
__le16 capab = mgmt -> u . probe_resp . capab_info ; 

if ( ieee80211_is_s1g_beacon ( mgmt -> frame_control ) ) 
return 0 ; 

if ( country_ie && 
( capab & cpu_to_le16 ( wlan_capability_spectrum_mgmt ) || 
capab & cpu_to_le16 ( wlan_capability_radio_measure ) ) ) { 
has_80211h_pwr = ieee80211_find_80211h_pwr_constr ( 
sdata , channel , country_ie , country_ie_len , 
pwr_constr_ie , & chan_pwr , & pwr_reduction_80211h ) ; 
pwr_level_80211h = 
max_t ( int , 0 , chan_pwr - pwr_reduction_80211h ) ; 
} 

if ( cisco_dtpc_ie ) { 
ieee80211_find_cisco_dtpc ( 
sdata , channel , cisco_dtpc_ie , & pwr_level_cisco ) ; 
has_cisco_pwr = true ; 
} 

if ( ! has_80211h_pwr && ! has_cisco_pwr ) 
return 0 ; 




if ( has_80211h_pwr && 
( ! has_cisco_pwr || pwr_level_80211h <= pwr_level_cisco ) ) { 
new_ap_level = pwr_level_80211h ; 

if ( link -> ap_power_level == new_ap_level ) 
return 0 ; 

sdata_dbg ( sdata , 
"limiting tx power to %d (%d - %d) dbm as advertised by %pm\n" , 
pwr_level_80211h , chan_pwr , pwr_reduction_80211h , 
link -> u . mgd . bssid ) ; 
} else { 
new_ap_level = pwr_level_cisco ; 

if ( link -> ap_power_level == new_ap_level ) 
return 0 ; 

sdata_dbg ( sdata , 
"limiting tx power to %d dbm as advertised by %pm\n" , 
pwr_level_cisco , link -> u . mgd . bssid ) ; 
} 

link -> ap_power_level = new_ap_level ; 
if ( __ieee80211_recalc_txpower ( sdata ) ) 
return bss_changed_txpower ; 
return 0 ; 
} 


static void ieee80211_enable_ps ( struct ieee80211_local * local , 
struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_conf * conf = & local -> hw . conf ; 





if ( local -> scanning ) 
return ; 

if ( conf -> dynamic_ps_timeout > 0 && 
! ieee80211_hw_check ( & local -> hw , supports_dynamic_ps ) ) { 
mod_timer ( & local -> dynamic_ps_timer , jiffies + 
msecs_to_jiffies ( conf -> dynamic_ps_timeout ) ) ; 
} else { 
if ( ieee80211_hw_check ( & local -> hw , ps_nullfunc_stack ) ) 
ieee80211_send_nullfunc ( local , sdata , true ) ; 

if ( ieee80211_hw_check ( & local -> hw , ps_nullfunc_stack ) && 
ieee80211_hw_check ( & local -> hw , reports_tx_ack_status ) ) 
return ; 

conf -> flags |= ieee80211_conf_ps ; 
ieee80211_hw_config ( local , ieee80211_conf_change_ps ) ; 
} 
} 

static void ieee80211_change_ps ( struct ieee80211_local * local ) 
{ 
struct ieee80211_conf * conf = & local -> hw . conf ; 

if ( local -> ps_sdata ) { 
ieee80211_enable_ps ( local , local -> ps_sdata ) ; 
} else if ( conf -> flags & ieee80211_conf_ps ) { 
conf -> flags &= ~ ieee80211_conf_ps ; 
ieee80211_hw_config ( local , ieee80211_conf_change_ps ) ; 
del_timer_sync ( & local -> dynamic_ps_timer ) ; 
cancel_work_sync ( & local -> dynamic_ps_enable_work ) ; 
} 
} 

static bool ieee80211_powersave_allowed ( struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_if_managed * mgd = & sdata -> u . mgd ; 
struct sta_info * sta = null ; 
bool authorized = false ; 

if ( ! mgd -> powersave ) 
return false ; 

if ( mgd -> broken_ap ) 
return false ; 

if ( ! mgd -> associated ) 
return false ; 

if ( mgd -> flags & ieee80211_sta_connection_poll ) 
return false ; 

if ( ! ( local -> hw . wiphy -> flags & wiphy_flag_supports_mlo ) && 
! sdata -> deflink . u . mgd . have_beacon ) 
return false ; 

rcu_read_lock ( ) ; 
sta = sta_info_get ( sdata , sdata -> vif . cfg . ap_addr ) ; 
if ( sta ) 
authorized = test_sta_flag ( sta , wlan_sta_authorized ) ; 
rcu_read_unlock ( ) ; 

return authorized ; 
} 


void ieee80211_recalc_ps ( struct ieee80211_local * local ) 
{ 
struct ieee80211_sub_if_data * sdata , * found = null ; 
int count = 0 ; 
int timeout ; 

if ( ! ieee80211_hw_check ( & local -> hw , supports_ps ) || 
ieee80211_hw_check ( & local -> hw , supports_dynamic_ps ) ) { 
local -> ps_sdata = null ; 
return ; 
} 

list_for_each_entry ( sdata , & local -> interfaces , list ) { 
if ( ! ieee80211_sdata_running ( sdata ) ) 
continue ; 
if ( sdata -> vif . type == nl80211_iftype_ap ) { 




count = 0 ; 
break ; 
} 
if ( sdata -> vif . type != nl80211_iftype_station ) 
continue ; 
found = sdata ; 
count ++ ; 
} 

if ( count == 1 && ieee80211_powersave_allowed ( found ) ) { 
u8 dtimper = found -> deflink . u . mgd . dtim_period ; 

timeout = local -> dynamic_ps_forced_timeout ; 
if ( timeout < 0 ) 
timeout = 100 ; 
local -> hw . conf . dynamic_ps_timeout = timeout ; 


if ( ! dtimper ) 
dtimper = 1 ; 

local -> hw . conf . ps_dtim_period = dtimper ; 
local -> ps_sdata = found ; 
} else { 
local -> ps_sdata = null ; 
} 

ieee80211_change_ps ( local ) ; 
} 

void ieee80211_recalc_ps_vif ( struct ieee80211_sub_if_data * sdata ) 
{ 
bool ps_allowed = ieee80211_powersave_allowed ( sdata ) ; 

if ( sdata -> vif . cfg . ps != ps_allowed ) { 
sdata -> vif . cfg . ps = ps_allowed ; 
ieee80211_vif_cfg_change_notify ( sdata , bss_changed_ps ) ; 
} 
} 

void ieee80211_dynamic_ps_disable_work ( struct work_struct * work ) 
{ 
struct ieee80211_local * local = 
container_of ( work , struct ieee80211_local , 
dynamic_ps_disable_work ) ; 

if ( local -> hw . conf . flags & ieee80211_conf_ps ) { 
local -> hw . conf . flags &= ~ ieee80211_conf_ps ; 
ieee80211_hw_config ( local , ieee80211_conf_change_ps ) ; 
} 

ieee80211_wake_queues_by_reason ( & local -> hw , 
ieee80211_max_queue_map , 
ieee80211_queue_stop_reason_ps , 
false ) ; 
} 

void ieee80211_dynamic_ps_enable_work ( struct work_struct * work ) 
{ 
struct ieee80211_local * local = 
container_of ( work , struct ieee80211_local , 
dynamic_ps_enable_work ) ; 
struct ieee80211_sub_if_data * sdata = local -> ps_sdata ; 
struct ieee80211_if_managed * ifmgd ; 
unsigned long flags ; 
int q ; 


if ( ! sdata ) 
return ; 

ifmgd = & sdata -> u . mgd ; 

if ( local -> hw . conf . flags & ieee80211_conf_ps ) 
return ; 

if ( local -> hw . conf . dynamic_ps_timeout > 0 ) { 

if ( drv_tx_frames_pending ( local ) ) { 
mod_timer ( & local -> dynamic_ps_timer , jiffies + 
msecs_to_jiffies ( 
local -> hw . conf . dynamic_ps_timeout ) ) ; 
return ; 
} 






spin_lock_irqsave ( & local -> queue_stop_reason_lock , flags ) ; 
for ( q = 0 ; q < local -> hw . queues ; q ++ ) { 
if ( local -> queue_stop_reasons [ q ] ) { 
spin_unlock_irqrestore ( & local -> queue_stop_reason_lock , 
flags ) ; 
mod_timer ( & local -> dynamic_ps_timer , jiffies + 
msecs_to_jiffies ( 
local -> hw . conf . dynamic_ps_timeout ) ) ; 
return ; 
} 
} 
spin_unlock_irqrestore ( & local -> queue_stop_reason_lock , flags ) ; 
} 

if ( ieee80211_hw_check ( & local -> hw , ps_nullfunc_stack ) && 
! ( ifmgd -> flags & ieee80211_sta_nullfunc_acked ) ) { 
if ( drv_tx_frames_pending ( local ) ) { 
mod_timer ( & local -> dynamic_ps_timer , jiffies + 
msecs_to_jiffies ( 
local -> hw . conf . dynamic_ps_timeout ) ) ; 
} else { 
ieee80211_send_nullfunc ( local , sdata , true ) ; 

ieee80211_flush_queues ( local , sdata , false ) ; 
} 
} 

if ( ! ( ieee80211_hw_check ( & local -> hw , reports_tx_ack_status ) && 
ieee80211_hw_check ( & local -> hw , ps_nullfunc_stack ) ) || 
( ifmgd -> flags & ieee80211_sta_nullfunc_acked ) ) { 
ifmgd -> flags &= ~ ieee80211_sta_nullfunc_acked ; 
local -> hw . conf . flags |= ieee80211_conf_ps ; 
ieee80211_hw_config ( local , ieee80211_conf_change_ps ) ; 
} 
} 

void ieee80211_dynamic_ps_timer ( struct timer_list * t ) 
{ 
struct ieee80211_local * local = from_timer ( local , t , dynamic_ps_timer ) ; 

ieee80211_queue_work ( & local -> hw , & local -> dynamic_ps_enable_work ) ; 
} 

void ieee80211_dfs_cac_timer_work ( struct work_struct * work ) 
{ 
struct delayed_work * delayed_work = to_delayed_work ( work ) ; 
struct ieee80211_link_data * link = 
container_of ( delayed_work , struct ieee80211_link_data , 
dfs_cac_timer_work ) ; 
struct cfg80211_chan_def chandef = link -> conf -> chandef ; 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 

mutex_lock ( & sdata -> local -> mtx ) ; 
if ( sdata -> wdev . cac_started ) { 
ieee80211_link_release_channel ( link ) ; 
cfg80211_cac_event ( sdata -> dev , & chandef , 
nl80211_radar_cac_finished , 
gfp_kernel ) ; 
} 
mutex_unlock ( & sdata -> local -> mtx ) ; 
} 

static bool 
__ieee80211_sta_handle_tspec_ac_params ( struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
bool ret = false ; 
int ac ; 

if ( local -> hw . queues < ieee80211_num_acs ) 
return false ; 

for ( ac = 0 ; ac < ieee80211_num_acs ; ac ++ ) { 
struct ieee80211_sta_tx_tspec * tx_tspec = & ifmgd -> tx_tspec [ ac ] ; 
int non_acm_ac ; 
unsigned long now = jiffies ; 

if ( tx_tspec -> action == tx_tspec_action_none && 
tx_tspec -> admitted_time && 
time_after ( now , tx_tspec -> time_slice_start + hz ) ) { 
tx_tspec -> consumed_tx_time = 0 ; 
tx_tspec -> time_slice_start = now ; 

if ( tx_tspec -> downgraded ) 
tx_tspec -> action = 
tx_tspec_action_stop_downgrade ; 
} 

switch ( tx_tspec -> action ) { 
case tx_tspec_action_stop_downgrade : 

if ( drv_conf_tx ( local , & sdata -> deflink , ac , 
& sdata -> deflink . tx_conf [ ac ] ) ) 
link_err ( & sdata -> deflink , 
"failed to set tx queue parameters for queue %d\n" , 
ac ) ; 
tx_tspec -> action = tx_tspec_action_none ; 
tx_tspec -> downgraded = false ; 
ret = true ; 
break ; 
case tx_tspec_action_downgrade : 
if ( time_after ( now , tx_tspec -> time_slice_start + hz ) ) { 
tx_tspec -> action = tx_tspec_action_none ; 
ret = true ; 
break ; 
} 

for ( non_acm_ac = ac + 1 ; 
non_acm_ac < ieee80211_num_acs ; 
non_acm_ac ++ ) 
if ( ! ( sdata -> wmm_acm & bit ( 7 - 2 * non_acm_ac ) ) ) 
break ; 







if ( non_acm_ac >= ieee80211_num_acs ) 
non_acm_ac = ieee80211_ac_bk ; 
if ( drv_conf_tx ( local , & sdata -> deflink , ac , 
& sdata -> deflink . tx_conf [ non_acm_ac ] ) ) 
link_err ( & sdata -> deflink , 
"failed to set tx queue parameters for queue %d\n" , 
ac ) ; 
tx_tspec -> action = tx_tspec_action_none ; 
ret = true ; 
schedule_delayed_work ( & ifmgd -> tx_tspec_wk , 
tx_tspec -> time_slice_start + hz - now + 1 ) ; 
break ; 
case tx_tspec_action_none : 

break ; 
} 
} 

return ret ; 
} 

void ieee80211_sta_handle_tspec_ac_params ( struct ieee80211_sub_if_data * sdata ) 
{ 
if ( __ieee80211_sta_handle_tspec_ac_params ( sdata ) ) 
ieee80211_link_info_change_notify ( sdata , & sdata -> deflink , 
bss_changed_qos ) ; 
} 

static void ieee80211_sta_handle_tspec_ac_params_wk ( struct work_struct * work ) 
{ 
struct ieee80211_sub_if_data * sdata ; 

sdata = container_of ( work , struct ieee80211_sub_if_data , 
u . mgd . tx_tspec_wk . work ) ; 
ieee80211_sta_handle_tspec_ac_params ( sdata ) ; 
} 

void ieee80211_mgd_set_link_qos_params ( struct ieee80211_link_data * link ) 
{ 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct ieee80211_tx_queue_params * params = link -> tx_conf ; 
u8 ac ; 

for ( ac = 0 ; ac < ieee80211_num_acs ; ac ++ ) { 
mlme_dbg ( sdata , 
"wmm ac=%d acm=%d aifs=%d cwmin=%d cwmax=%d txop=%d uapsd=%d, downgraded=%d\n" , 
ac , params [ ac ] . acm , 
params [ ac ] . aifs , params [ ac ] . cw_min , params [ ac ] . cw_max , 
params [ ac ] . txop , params [ ac ] . uapsd , 
ifmgd -> tx_tspec [ ac ] . downgraded ) ; 
if ( ! ifmgd -> tx_tspec [ ac ] . downgraded && 
drv_conf_tx ( local , link , ac , & params [ ac ] ) ) 
link_err ( link , 
"failed to set tx queue parameters for ac %d\n" , 
ac ) ; 
} 
} 


static bool 
ieee80211_sta_wmm_params ( struct ieee80211_local * local , 
struct ieee80211_link_data * link , 
const u8 * wmm_param , size_t wmm_param_len , 
const struct ieee80211_mu_edca_param_set * mu_edca ) 
{ 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 
struct ieee80211_tx_queue_params params [ ieee80211_num_acs ] ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
size_t left ; 
int count , mu_edca_count , ac ; 
const u8 * pos ; 
u8 uapsd_queues = 0 ; 

if ( ! local -> ops -> conf_tx ) 
return false ; 

if ( local -> hw . queues < ieee80211_num_acs ) 
return false ; 

if ( ! wmm_param ) 
return false ; 

if ( wmm_param_len < 8 || wmm_param [ 5 ] != 1 ) 
return false ; 

if ( ifmgd -> flags & ieee80211_sta_uapsd_enabled ) 
uapsd_queues = ifmgd -> uapsd_queues ; 

count = wmm_param [ 6 ] & 0x0f ; 




mu_edca_count = mu_edca ? mu_edca -> mu_qos_info & 0x0f : - 1 ; 
if ( count == link -> u . mgd . wmm_last_param_set && 
mu_edca_count == link -> u . mgd . mu_edca_last_param_set ) 
return false ; 
link -> u . mgd . wmm_last_param_set = count ; 
link -> u . mgd . mu_edca_last_param_set = mu_edca_count ; 

pos = wmm_param + 8 ; 
left = wmm_param_len - 8 ; 

memset ( & params , 0 , sizeof ( params ) ) ; 

sdata -> wmm_acm = 0 ; 
for ( ; left >= 4 ; left -= 4 , pos += 4 ) { 
int aci = ( pos [ 0 ] >> 5 ) & 0x03 ; 
int acm = ( pos [ 0 ] >> 4 ) & 0x01 ; 
bool uapsd = false ; 

switch ( aci ) { 
case 1 : 
ac = ieee80211_ac_bk ; 
if ( acm ) 
sdata -> wmm_acm |= bit ( 1 ) | bit ( 2 ) ; 
if ( uapsd_queues & ieee80211_wmm_ie_sta_qosinfo_ac_bk ) 
uapsd = true ; 
params [ ac ] . mu_edca = ! ! mu_edca ; 
if ( mu_edca ) 
params [ ac ] . mu_edca_param_rec = mu_edca -> ac_bk ; 
break ; 
case 2 : 
ac = ieee80211_ac_vi ; 
if ( acm ) 
sdata -> wmm_acm |= bit ( 4 ) | bit ( 5 ) ; 
if ( uapsd_queues & ieee80211_wmm_ie_sta_qosinfo_ac_vi ) 
uapsd = true ; 
params [ ac ] . mu_edca = ! ! mu_edca ; 
if ( mu_edca ) 
params [ ac ] . mu_edca_param_rec = mu_edca -> ac_vi ; 
break ; 
case 3 : 
ac = ieee80211_ac_vo ; 
if ( acm ) 
sdata -> wmm_acm |= bit ( 6 ) | bit ( 7 ) ; 
if ( uapsd_queues & ieee80211_wmm_ie_sta_qosinfo_ac_vo ) 
uapsd = true ; 
params [ ac ] . mu_edca = ! ! mu_edca ; 
if ( mu_edca ) 
params [ ac ] . mu_edca_param_rec = mu_edca -> ac_vo ; 
break ; 
case 0 : 
default : 
ac = ieee80211_ac_be ; 
if ( acm ) 
sdata -> wmm_acm |= bit ( 0 ) | bit ( 3 ) ; 
if ( uapsd_queues & ieee80211_wmm_ie_sta_qosinfo_ac_be ) 
uapsd = true ; 
params [ ac ] . mu_edca = ! ! mu_edca ; 
if ( mu_edca ) 
params [ ac ] . mu_edca_param_rec = mu_edca -> ac_be ; 
break ; 
} 

params [ ac ] . aifs = pos [ 0 ] & 0x0f ; 

if ( params [ ac ] . aifs < 2 ) { 
sdata_info ( sdata , 
"ap has invalid wmm params (aifsn=%d for aci %d), will use 2\n" , 
params [ ac ] . aifs , aci ) ; 
params [ ac ] . aifs = 2 ; 
} 
params [ ac ] . cw_max = ecw2cw ( ( pos [ 1 ] & 0xf0 ) >> 4 ) ; 
params [ ac ] . cw_min = ecw2cw ( pos [ 1 ] & 0x0f ) ; 
params [ ac ] . txop = get_unaligned_le16 ( pos + 2 ) ; 
params [ ac ] . acm = acm ; 
params [ ac ] . uapsd = uapsd ; 

if ( params [ ac ] . cw_min == 0 || 
params [ ac ] . cw_min > params [ ac ] . cw_max ) { 
sdata_info ( sdata , 
"ap has invalid wmm params (cwmin/max=%d/%d for aci %d), using defaults\n" , 
params [ ac ] . cw_min , params [ ac ] . cw_max , aci ) ; 
return false ; 
} 
ieee80211_regulatory_limit_wmm_params ( sdata , & params [ ac ] , ac ) ; 
} 


for ( ac = 0 ; ac < ieee80211_num_acs ; ac ++ ) { 
if ( params [ ac ] . cw_min == 0 ) { 
sdata_info ( sdata , 
"ap has invalid wmm params (missing ac %d), using defaults\n" , 
ac ) ; 
return false ; 
} 
} 

for ( ac = 0 ; ac < ieee80211_num_acs ; ac ++ ) 
link -> tx_conf [ ac ] = params [ ac ] ; 

ieee80211_mgd_set_link_qos_params ( link ) ; 


link -> conf -> qos = true ; 
return true ; 
} 

static void __ieee80211_stop_poll ( struct ieee80211_sub_if_data * sdata ) 
{ 
lockdep_assert_held ( & sdata -> local -> mtx ) ; 

sdata -> u . mgd . flags &= ~ ieee80211_sta_connection_poll ; 
ieee80211_run_deferred_scan ( sdata -> local ) ; 
} 

static void ieee80211_stop_poll ( struct ieee80211_sub_if_data * sdata ) 
{ 
mutex_lock ( & sdata -> local -> mtx ) ; 
__ieee80211_stop_poll ( sdata ) ; 
mutex_unlock ( & sdata -> local -> mtx ) ; 
} 

static u32 ieee80211_handle_bss_capability ( struct ieee80211_link_data * link , 
u16 capab , bool erp_valid , u8 erp ) 
{ 
struct ieee80211_bss_conf * bss_conf = link -> conf ; 
struct ieee80211_supported_band * sband ; 
u32 changed = 0 ; 
bool use_protection ; 
bool use_short_preamble ; 
bool use_short_slot ; 

sband = ieee80211_get_link_sband ( link ) ; 
if ( ! sband ) 
return changed ; 

if ( erp_valid ) { 
use_protection = ( erp & wlan_erp_use_protection ) != 0 ; 
use_short_preamble = ( erp & wlan_erp_barker_preamble ) == 0 ; 
} else { 
use_protection = false ; 
use_short_preamble = ! ! ( capab & wlan_capability_short_preamble ) ; 
} 

use_short_slot = ! ! ( capab & wlan_capability_short_slot_time ) ; 
if ( sband -> band == nl80211_band_5ghz || 
sband -> band == nl80211_band_6ghz ) 
use_short_slot = true ; 

if ( use_protection != bss_conf -> use_cts_prot ) { 
bss_conf -> use_cts_prot = use_protection ; 
changed |= bss_changed_erp_cts_prot ; 
} 

if ( use_short_preamble != bss_conf -> use_short_preamble ) { 
bss_conf -> use_short_preamble = use_short_preamble ; 
changed |= bss_changed_erp_preamble ; 
} 

if ( use_short_slot != bss_conf -> use_short_slot ) { 
bss_conf -> use_short_slot = use_short_slot ; 
changed |= bss_changed_erp_slot ; 
} 

return changed ; 
} 

static u32 ieee80211_link_set_associated ( struct ieee80211_link_data * link , 
struct cfg80211_bss * cbss ) 
{ 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 
struct ieee80211_bss_conf * bss_conf = link -> conf ; 
struct ieee80211_bss * bss = ( void * ) cbss -> priv ; 
u32 changed = bss_changed_qos ; 


sdata -> u . mgd . beacon_timeout = 
usecs_to_jiffies ( ieee80211_tu_to_usec ( beacon_loss_count * 
bss_conf -> beacon_int ) ) ; 

changed |= ieee80211_handle_bss_capability ( link , 
bss_conf -> assoc_capability , 
bss -> has_erp_value , 
bss -> erp_value ) ; 

ieee80211_check_rate_mask ( link ) ; 

link -> u . mgd . bss = cbss ; 
memcpy ( link -> u . mgd . bssid , cbss -> bssid , eth_alen ) ; 

if ( sdata -> vif . p2p || 
sdata -> vif . driver_flags & ieee80211_vif_get_noa_update ) { 
const struct cfg80211_bss_ies * ies ; 

rcu_read_lock ( ) ; 
ies = rcu_dereference ( cbss -> ies ) ; 
if ( ies ) { 
int ret ; 

ret = cfg80211_get_p2p_attr ( 
ies -> data , ies -> len , 
ieee80211_p2p_attr_absence_notice , 
( u8 * ) & bss_conf -> p2p_noa_attr , 
sizeof ( bss_conf -> p2p_noa_attr ) ) ; 
if ( ret >= 2 ) { 
link -> u . mgd . p2p_noa_index = 
bss_conf -> p2p_noa_attr . index ; 
changed |= bss_changed_p2p_ps ; 
} 
} 
rcu_read_unlock ( ) ; 
} 

if ( link -> u . mgd . have_beacon ) { 
bss_conf -> beacon_rate = bss -> beacon_rate ; 
changed |= bss_changed_beacon_info ; 
} else { 
bss_conf -> beacon_rate = null ; 
} 


if ( sdata -> vif . driver_flags & ieee80211_vif_supports_cqm_rssi && 
bss_conf -> cqm_rssi_thold ) 
changed |= bss_changed_cqm ; 

return changed ; 
} 

static void ieee80211_set_associated ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_mgd_assoc_data * assoc_data , 
u64 changed [ ieee80211_mld_max_num_links ] ) 
{ 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_vif_cfg * vif_cfg = & sdata -> vif . cfg ; 
u64 vif_changed = bss_changed_assoc ; 
unsigned int link_id ; 

sdata -> u . mgd . associated = true ; 

for ( link_id = 0 ; link_id < ieee80211_mld_max_num_links ; link_id ++ ) { 
struct cfg80211_bss * cbss = assoc_data -> link [ link_id ] . bss ; 
struct ieee80211_link_data * link ; 

if ( ! cbss || 
assoc_data -> link [ link_id ] . status != wlan_status_success ) 
continue ; 

link = sdata_dereference ( sdata -> link [ link_id ] , sdata ) ; 
if ( warn_on ( ! link ) ) 
return ; 

changed [ link_id ] |= ieee80211_link_set_associated ( link , cbss ) ; 
} 


ieee80211_stop_poll ( sdata ) ; 

ieee80211_led_assoc ( local , 1 ) ; 

vif_cfg -> assoc = 1 ; 


if ( vif_cfg -> arp_addr_cnt ) 
vif_changed |= bss_changed_arp_filter ; 

if ( sdata -> vif . valid_links ) { 
for ( link_id = 0 ; 
link_id < ieee80211_mld_max_num_links ; 
link_id ++ ) { 
struct ieee80211_link_data * link ; 
struct cfg80211_bss * cbss = assoc_data -> link [ link_id ] . bss ; 

if ( ! cbss || 
assoc_data -> link [ link_id ] . status != wlan_status_success ) 
continue ; 

link = sdata_dereference ( sdata -> link [ link_id ] , sdata ) ; 
if ( warn_on ( ! link ) ) 
return ; 

ieee80211_link_info_change_notify ( sdata , link , 
changed [ link_id ] ) ; 

ieee80211_recalc_smps ( sdata , link ) ; 
} 

ieee80211_vif_cfg_change_notify ( sdata , vif_changed ) ; 
} else { 
ieee80211_bss_info_change_notify ( sdata , 
vif_changed | changed [ 0 ] ) ; 
} 

mutex_lock ( & local -> iflist_mtx ) ; 
ieee80211_recalc_ps ( local ) ; 
mutex_unlock ( & local -> iflist_mtx ) ; 


if ( ! sdata -> vif . valid_links ) 
ieee80211_recalc_smps ( sdata , & sdata -> deflink ) ; 
ieee80211_recalc_ps_vif ( sdata ) ; 

netif_carrier_on ( sdata -> dev ) ; 
} 

static void ieee80211_set_disassoc ( struct ieee80211_sub_if_data * sdata , 
u16 stype , u16 reason , bool tx , 
u8 * frame_buf ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct ieee80211_local * local = sdata -> local ; 
unsigned int link_id ; 
u32 changed = 0 ; 
struct ieee80211_prep_tx_info info = { 
. subtype = stype , 
} ; 

sdata_assert_lock ( sdata ) ; 

if ( warn_on_once ( tx && ! frame_buf ) ) 
return ; 

if ( warn_on ( ! ifmgd -> associated ) ) 
return ; 

ieee80211_stop_poll ( sdata ) ; 

ifmgd -> associated = false ; 


sdata -> deflink . u . mgd . bss = null ; 

netif_carrier_off ( sdata -> dev ) ; 






if ( local -> hw . conf . flags & ieee80211_conf_ps ) { 
local -> hw . conf . flags &= ~ ieee80211_conf_ps ; 
ieee80211_hw_config ( local , ieee80211_conf_change_ps ) ; 
} 
local -> ps_sdata = null ; 


ieee80211_recalc_ps_vif ( sdata ) ; 


synchronize_net ( ) ; 







if ( tx ) 
ieee80211_flush_queues ( local , sdata , true ) ; 


if ( tx || frame_buf ) { 






if ( ieee80211_hw_check ( & local -> hw , deauth_need_mgd_tx_prep ) && 
! sdata -> deflink . u . mgd . have_beacon ) { 
drv_mgd_prepare_tx ( sdata -> local , sdata , & info ) ; 
} 

ieee80211_send_deauth_disassoc ( sdata , sdata -> vif . cfg . ap_addr , 
sdata -> vif . cfg . ap_addr , stype , 
reason , tx , frame_buf ) ; 
} 


if ( tx ) 
ieee80211_flush_queues ( local , sdata , false ) ; 

drv_mgd_complete_tx ( sdata -> local , sdata , & info ) ; 


eth_zero_addr ( sdata -> deflink . u . mgd . bssid ) ; 
eth_zero_addr ( sdata -> vif . cfg . ap_addr ) ; 

sdata -> vif . cfg . ssid_len = 0 ; 


sta_info_flush ( sdata ) ; 


if ( ! sdata -> vif . valid_links ) 
changed |= ieee80211_reset_erp_info ( sdata ) ; 

ieee80211_led_assoc ( local , 0 ) ; 
changed |= bss_changed_assoc ; 
sdata -> vif . cfg . assoc = false ; 

sdata -> deflink . u . mgd . p2p_noa_index = - 1 ; 
memset ( & sdata -> vif . bss_conf . p2p_noa_attr , 0 , 
sizeof ( sdata -> vif . bss_conf . p2p_noa_attr ) ) ; 


memset ( & ifmgd -> ht_capa , 0 , sizeof ( ifmgd -> ht_capa ) ) ; 
memset ( & ifmgd -> ht_capa_mask , 0 , sizeof ( ifmgd -> ht_capa_mask ) ) ; 
memset ( & ifmgd -> vht_capa , 0 , sizeof ( ifmgd -> vht_capa ) ) ; 
memset ( & ifmgd -> vht_capa_mask , 0 , sizeof ( ifmgd -> vht_capa_mask ) ) ; 





memset ( sdata -> vif . bss_conf . mu_group . membership , 0 , 
sizeof ( sdata -> vif . bss_conf . mu_group . membership ) ) ; 
memset ( sdata -> vif . bss_conf . mu_group . position , 0 , 
sizeof ( sdata -> vif . bss_conf . mu_group . position ) ) ; 
if ( ! sdata -> vif . valid_links ) 
changed |= bss_changed_mu_groups ; 
sdata -> vif . bss_conf . mu_mimo_owner = false ; 

sdata -> deflink . ap_power_level = ieee80211_unset_power_level ; 

del_timer_sync ( & local -> dynamic_ps_timer ) ; 
cancel_work_sync ( & local -> dynamic_ps_enable_work ) ; 


if ( sdata -> vif . cfg . arp_addr_cnt ) 
changed |= bss_changed_arp_filter ; 

sdata -> vif . bss_conf . qos = false ; 
if ( ! sdata -> vif . valid_links ) { 
changed |= bss_changed_qos ; 

changed |= bss_changed_bssid | bss_changed_ht ; 
ieee80211_bss_info_change_notify ( sdata , changed ) ; 
} else { 
ieee80211_vif_cfg_change_notify ( sdata , changed ) ; 
} 


ieee80211_set_wmm_default ( & sdata -> deflink , false , false ) ; 

del_timer_sync ( & sdata -> u . mgd . conn_mon_timer ) ; 
del_timer_sync ( & sdata -> u . mgd . bcn_mon_timer ) ; 
del_timer_sync ( & sdata -> u . mgd . timer ) ; 
del_timer_sync ( & sdata -> deflink . u . mgd . chswitch_timer ) ; 

sdata -> vif . bss_conf . dtim_period = 0 ; 
sdata -> vif . bss_conf . beacon_rate = null ; 

sdata -> deflink . u . mgd . have_beacon = false ; 
sdata -> deflink . u . mgd . tracking_signal_avg = false ; 
sdata -> deflink . u . mgd . disable_wmm_tracking = false ; 

ifmgd -> flags = 0 ; 
sdata -> deflink . u . mgd . conn_flags = 0 ; 
mutex_lock ( & local -> mtx ) ; 

for ( link_id = 0 ; link_id < array_size ( sdata -> link ) ; link_id ++ ) { 
struct ieee80211_link_data * link ; 

link = sdata_dereference ( sdata -> link [ link_id ] , sdata ) ; 
if ( ! link ) 
continue ; 
ieee80211_link_release_channel ( link ) ; 
} 

sdata -> vif . bss_conf . csa_active = false ; 
sdata -> deflink . u . mgd . csa_waiting_bcn = false ; 
sdata -> deflink . u . mgd . csa_ignored_same_chan = false ; 
if ( sdata -> deflink . csa_block_tx ) { 
ieee80211_wake_vif_queues ( local , sdata , 
ieee80211_queue_stop_reason_csa ) ; 
sdata -> deflink . csa_block_tx = false ; 
} 
mutex_unlock ( & local -> mtx ) ; 


memset ( ifmgd -> tx_tspec , 0 , sizeof ( ifmgd -> tx_tspec ) ) ; 
cancel_delayed_work_sync ( & ifmgd -> tx_tspec_wk ) ; 

sdata -> vif . bss_conf . pwr_reduction = 0 ; 
sdata -> vif . bss_conf . tx_pwr_env_num = 0 ; 
memset ( sdata -> vif . bss_conf . tx_pwr_env , 0 , 
sizeof ( sdata -> vif . bss_conf . tx_pwr_env ) ) ; 

ieee80211_vif_set_links ( sdata , 0 ) ; 
} 

static void ieee80211_reset_ap_probe ( struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct ieee80211_local * local = sdata -> local ; 

mutex_lock ( & local -> mtx ) ; 
if ( ! ( ifmgd -> flags & ieee80211_sta_connection_poll ) ) 
goto out ; 

__ieee80211_stop_poll ( sdata ) ; 

mutex_lock ( & local -> iflist_mtx ) ; 
ieee80211_recalc_ps ( local ) ; 
mutex_unlock ( & local -> iflist_mtx ) ; 

if ( ieee80211_hw_check ( & sdata -> local -> hw , connection_monitor ) ) 
goto out ; 






ieee80211_sta_reset_beacon_monitor ( sdata ) ; 

mod_timer ( & ifmgd -> conn_mon_timer , 
round_jiffies_up ( jiffies + 
ieee80211_connection_idle_time ) ) ; 
out : 
mutex_unlock ( & local -> mtx ) ; 
} 

static void ieee80211_sta_tx_wmm_ac_notify ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_hdr * hdr , 
u16 tx_time ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
u16 tid ; 
int ac ; 
struct ieee80211_sta_tx_tspec * tx_tspec ; 
unsigned long now = jiffies ; 

if ( ! ieee80211_is_data_qos ( hdr -> frame_control ) ) 
return ; 

tid = ieee80211_get_tid ( hdr ) ; 
ac = ieee80211_ac_from_tid ( tid ) ; 
tx_tspec = & ifmgd -> tx_tspec [ ac ] ; 

if ( likely ( ! tx_tspec -> admitted_time ) ) 
return ; 

if ( time_after ( now , tx_tspec -> time_slice_start + hz ) ) { 
tx_tspec -> consumed_tx_time = 0 ; 
tx_tspec -> time_slice_start = now ; 

if ( tx_tspec -> downgraded ) { 
tx_tspec -> action = tx_tspec_action_stop_downgrade ; 
schedule_delayed_work ( & ifmgd -> tx_tspec_wk , 0 ) ; 
} 
} 

if ( tx_tspec -> downgraded ) 
return ; 

tx_tspec -> consumed_tx_time += tx_time ; 

if ( tx_tspec -> consumed_tx_time >= tx_tspec -> admitted_time ) { 
tx_tspec -> downgraded = true ; 
tx_tspec -> action = tx_tspec_action_downgrade ; 
schedule_delayed_work ( & ifmgd -> tx_tspec_wk , 0 ) ; 
} 
} 

void ieee80211_sta_tx_notify ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_hdr * hdr , bool ack , u16 tx_time ) 
{ 
ieee80211_sta_tx_wmm_ac_notify ( sdata , hdr , tx_time ) ; 

if ( ! ieee80211_is_any_nullfunc ( hdr -> frame_control ) || 
! sdata -> u . mgd . probe_send_count ) 
return ; 

if ( ack ) 
sdata -> u . mgd . probe_send_count = 0 ; 
else 
sdata -> u . mgd . nullfunc_failed = true ; 
ieee80211_queue_work ( & sdata -> local -> hw , & sdata -> work ) ; 
} 

static void ieee80211_mlme_send_probe_req ( struct ieee80211_sub_if_data * sdata , 
const u8 * src , const u8 * dst , 
const u8 * ssid , size_t ssid_len , 
struct ieee80211_channel * channel ) 
{ 
struct sk_buff * skb ; 

skb = ieee80211_build_probe_req ( sdata , src , dst , ( u32 ) - 1 , channel , 
ssid , ssid_len , null , 0 , 
ieee80211_probe_flag_directed ) ; 
if ( skb ) 
ieee80211_tx_skb ( sdata , skb ) ; 
} 

static void ieee80211_mgd_probe_ap_send ( struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
u8 * dst = sdata -> vif . cfg . ap_addr ; 
u8 unicast_limit = max ( 1 , max_probe_tries - 3 ) ; 
struct sta_info * sta ; 

if ( warn_on ( sdata -> vif . valid_links ) ) 
return ; 






if ( ifmgd -> probe_send_count >= unicast_limit ) 
dst = null ; 








ifmgd -> probe_send_count ++ ; 

if ( dst ) { 
mutex_lock ( & sdata -> local -> sta_mtx ) ; 
sta = sta_info_get ( sdata , dst ) ; 
if ( ! warn_on ( ! sta ) ) 
ieee80211_check_fast_rx ( sta ) ; 
mutex_unlock ( & sdata -> local -> sta_mtx ) ; 
} 

if ( ieee80211_hw_check ( & sdata -> local -> hw , reports_tx_ack_status ) ) { 
ifmgd -> nullfunc_failed = false ; 
ieee80211_send_nullfunc ( sdata -> local , sdata , false ) ; 
} else { 
ieee80211_mlme_send_probe_req ( sdata , sdata -> vif . addr , dst , 
sdata -> vif . cfg . ssid , 
sdata -> vif . cfg . ssid_len , 
sdata -> deflink . u . mgd . bss -> channel ) ; 
} 

ifmgd -> probe_timeout = jiffies + msecs_to_jiffies ( probe_wait_ms ) ; 
run_again ( sdata , ifmgd -> probe_timeout ) ; 
} 

static void ieee80211_mgd_probe_ap ( struct ieee80211_sub_if_data * sdata , 
bool beacon ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
bool already = false ; 

if ( warn_on ( sdata -> vif . valid_links ) ) 
return ; 

if ( ! ieee80211_sdata_running ( sdata ) ) 
return ; 

sdata_lock ( sdata ) ; 

if ( ! ifmgd -> associated ) 
goto out ; 

mutex_lock ( & sdata -> local -> mtx ) ; 

if ( sdata -> local -> tmp_channel || sdata -> local -> scanning ) { 
mutex_unlock ( & sdata -> local -> mtx ) ; 
goto out ; 
} 

if ( sdata -> local -> suspending ) { 

mutex_unlock ( & sdata -> local -> mtx ) ; 
ieee80211_reset_ap_probe ( sdata ) ; 
goto out ; 
} 

if ( beacon ) { 
mlme_dbg_ratelimited ( sdata , 
"detected beacon loss from ap (missed %d beacons) - probing\n" , 
beacon_loss_count ) ; 

ieee80211_cqm_beacon_loss_notify ( & sdata -> vif , gfp_kernel ) ; 
} 












if ( ifmgd -> flags & ieee80211_sta_connection_poll ) 
already = true ; 

ifmgd -> flags |= ieee80211_sta_connection_poll ; 

mutex_unlock ( & sdata -> local -> mtx ) ; 

if ( already ) 
goto out ; 

mutex_lock ( & sdata -> local -> iflist_mtx ) ; 
ieee80211_recalc_ps ( sdata -> local ) ; 
mutex_unlock ( & sdata -> local -> iflist_mtx ) ; 

ifmgd -> probe_send_count = 0 ; 
ieee80211_mgd_probe_ap_send ( sdata ) ; 
out : 
sdata_unlock ( sdata ) ; 
} 

struct sk_buff * ieee80211_ap_probereq_get ( struct ieee80211_hw * hw , 
struct ieee80211_vif * vif ) 
{ 
struct ieee80211_sub_if_data * sdata = vif_to_sdata ( vif ) ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct cfg80211_bss * cbss ; 
struct sk_buff * skb ; 
const struct element * ssid ; 
int ssid_len ; 

if ( warn_on ( sdata -> vif . type != nl80211_iftype_station || 
sdata -> vif . valid_links ) ) 
return null ; 

sdata_assert_lock ( sdata ) ; 

if ( ifmgd -> associated ) 
cbss = sdata -> deflink . u . mgd . bss ; 
else if ( ifmgd -> auth_data ) 
cbss = ifmgd -> auth_data -> bss ; 
else if ( ifmgd -> assoc_data && ifmgd -> assoc_data -> link [ 0 ] . bss ) 
cbss = ifmgd -> assoc_data -> link [ 0 ] . bss ; 
else 
return null ; 

rcu_read_lock ( ) ; 
ssid = ieee80211_bss_get_elem ( cbss , wlan_eid_ssid ) ; 
if ( warn_once ( ! ssid || ssid -> datalen > ieee80211_max_ssid_len , 
"invalid ssid element (len=%d)" , 
ssid ? ssid -> datalen : - 1 ) ) 
ssid_len = 0 ; 
else 
ssid_len = ssid -> datalen ; 

skb = ieee80211_build_probe_req ( sdata , sdata -> vif . addr , cbss -> bssid , 
( u32 ) - 1 , cbss -> channel , 
ssid -> data , ssid_len , 
null , 0 , ieee80211_probe_flag_directed ) ; 
rcu_read_unlock ( ) ; 

return skb ; 
} 
export_symbol ( ieee80211_ap_probereq_get ) ; 

static void ieee80211_report_disconnect ( struct ieee80211_sub_if_data * sdata , 
const u8 * buf , size_t len , bool tx , 
u16 reason , bool reconnect ) 
{ 
struct ieee80211_event event = { 
. type = mlme_event , 
. u . mlme . data = tx ? deauth_tx_event : deauth_rx_event , 
. u . mlme . reason = reason , 
} ; 

if ( tx ) 
cfg80211_tx_mlme_mgmt ( sdata -> dev , buf , len , reconnect ) ; 
else 
cfg80211_rx_mlme_mgmt ( sdata -> dev , buf , len ) ; 

drv_event_callback ( sdata -> local , sdata , & event ) ; 
} 

static void __ieee80211_disconnect ( struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
u8 frame_buf [ ieee80211_deauth_frame_len ] ; 
bool tx ; 

sdata_lock ( sdata ) ; 
if ( ! ifmgd -> associated ) { 
sdata_unlock ( sdata ) ; 
return ; 
} 


tx = sdata -> vif . valid_links || ! sdata -> deflink . csa_block_tx ; 

if ( ! ifmgd -> driver_disconnect ) { 
unsigned int link_id ; 








for ( link_id = 0 ; 
link_id < array_size ( sdata -> link ) ; 
link_id ++ ) { 
struct ieee80211_link_data * link ; 

link = sdata_dereference ( sdata -> link [ link_id ] , sdata ) ; 
if ( ! link ) 
continue ; 
cfg80211_unlink_bss ( local -> hw . wiphy , link -> u . mgd . bss ) ; 
link -> u . mgd . bss = null ; 
} 
} 

ieee80211_set_disassoc ( sdata , ieee80211_stype_deauth , 
ifmgd -> driver_disconnect ? 
wlan_reason_deauth_leaving : 
wlan_reason_disassoc_due_to_inactivity , 
tx , frame_buf ) ; 
mutex_lock ( & local -> mtx ) ; 

sdata -> vif . bss_conf . csa_active = false ; 
sdata -> deflink . u . mgd . csa_waiting_bcn = false ; 
if ( sdata -> deflink . csa_block_tx ) { 
ieee80211_wake_vif_queues ( local , sdata , 
ieee80211_queue_stop_reason_csa ) ; 
sdata -> deflink . csa_block_tx = false ; 
} 
mutex_unlock ( & local -> mtx ) ; 

ieee80211_report_disconnect ( sdata , frame_buf , sizeof ( frame_buf ) , tx , 
wlan_reason_disassoc_due_to_inactivity , 
ifmgd -> reconnect ) ; 
ifmgd -> reconnect = false ; 

sdata_unlock ( sdata ) ; 
} 

static void ieee80211_beacon_connection_loss_work ( struct work_struct * work ) 
{ 
struct ieee80211_sub_if_data * sdata = 
container_of ( work , struct ieee80211_sub_if_data , 
u . mgd . beacon_connection_loss_work ) ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 

if ( ifmgd -> connection_loss ) { 
sdata_info ( sdata , "connection to ap %pm lost\n" , 
sdata -> vif . cfg . ap_addr ) ; 
__ieee80211_disconnect ( sdata ) ; 
ifmgd -> connection_loss = false ; 
} else if ( ifmgd -> driver_disconnect ) { 
sdata_info ( sdata , 
"driver requested disconnection from ap %pm\n" , 
sdata -> vif . cfg . ap_addr ) ; 
__ieee80211_disconnect ( sdata ) ; 
ifmgd -> driver_disconnect = false ; 
} else { 
if ( ifmgd -> associated ) 
sdata -> deflink . u . mgd . beacon_loss_count ++ ; 
ieee80211_mgd_probe_ap ( sdata , true ) ; 
} 
} 

static void ieee80211_csa_connection_drop_work ( struct work_struct * work ) 
{ 
struct ieee80211_sub_if_data * sdata = 
container_of ( work , struct ieee80211_sub_if_data , 
u . mgd . csa_connection_drop_work ) ; 

__ieee80211_disconnect ( sdata ) ; 
} 

void ieee80211_beacon_loss ( struct ieee80211_vif * vif ) 
{ 
struct ieee80211_sub_if_data * sdata = vif_to_sdata ( vif ) ; 
struct ieee80211_hw * hw = & sdata -> local -> hw ; 

trace_api_beacon_loss ( sdata ) ; 

sdata -> u . mgd . connection_loss = false ; 
ieee80211_queue_work ( hw , & sdata -> u . mgd . beacon_connection_loss_work ) ; 
} 
export_symbol ( ieee80211_beacon_loss ) ; 

void ieee80211_connection_loss ( struct ieee80211_vif * vif ) 
{ 
struct ieee80211_sub_if_data * sdata = vif_to_sdata ( vif ) ; 
struct ieee80211_hw * hw = & sdata -> local -> hw ; 

trace_api_connection_loss ( sdata ) ; 

sdata -> u . mgd . connection_loss = true ; 
ieee80211_queue_work ( hw , & sdata -> u . mgd . beacon_connection_loss_work ) ; 
} 
export_symbol ( ieee80211_connection_loss ) ; 

void ieee80211_disconnect ( struct ieee80211_vif * vif , bool reconnect ) 
{ 
struct ieee80211_sub_if_data * sdata = vif_to_sdata ( vif ) ; 
struct ieee80211_hw * hw = & sdata -> local -> hw ; 

trace_api_disconnect ( sdata , reconnect ) ; 

if ( warn_on ( sdata -> vif . type != nl80211_iftype_station ) ) 
return ; 

sdata -> u . mgd . driver_disconnect = true ; 
sdata -> u . mgd . reconnect = reconnect ; 
ieee80211_queue_work ( hw , & sdata -> u . mgd . beacon_connection_loss_work ) ; 
} 
export_symbol ( ieee80211_disconnect ) ; 

static void ieee80211_destroy_auth_data ( struct ieee80211_sub_if_data * sdata , 
bool assoc ) 
{ 
struct ieee80211_mgd_auth_data * auth_data = sdata -> u . mgd . auth_data ; 

sdata_assert_lock ( sdata ) ; 

if ( ! assoc ) { 





del_timer_sync ( & sdata -> u . mgd . timer ) ; 
sta_info_destroy_addr ( sdata , auth_data -> ap_addr ) ; 


sdata -> deflink . u . mgd . conn_flags = 0 ; 
eth_zero_addr ( sdata -> deflink . u . mgd . bssid ) ; 
ieee80211_link_info_change_notify ( sdata , & sdata -> deflink , 
bss_changed_bssid ) ; 
sdata -> u . mgd . flags = 0 ; 

mutex_lock ( & sdata -> local -> mtx ) ; 
ieee80211_link_release_channel ( & sdata -> deflink ) ; 
ieee80211_vif_set_links ( sdata , 0 ) ; 
mutex_unlock ( & sdata -> local -> mtx ) ; 
} 

cfg80211_put_bss ( sdata -> local -> hw . wiphy , auth_data -> bss ) ; 
kfree ( auth_data ) ; 
sdata -> u . mgd . auth_data = null ; 
} 

enum assoc_status { 
assoc_success , 
assoc_rejected , 
assoc_timeout , 
assoc_abandon , 
} ; 

static void ieee80211_destroy_assoc_data ( struct ieee80211_sub_if_data * sdata , 
enum assoc_status status ) 
{ 
struct ieee80211_mgd_assoc_data * assoc_data = sdata -> u . mgd . assoc_data ; 

sdata_assert_lock ( sdata ) ; 

if ( status != assoc_success ) { 





del_timer_sync ( & sdata -> u . mgd . timer ) ; 
sta_info_destroy_addr ( sdata , assoc_data -> ap_addr ) ; 

sdata -> deflink . u . mgd . conn_flags = 0 ; 
eth_zero_addr ( sdata -> deflink . u . mgd . bssid ) ; 
ieee80211_link_info_change_notify ( sdata , & sdata -> deflink , 
bss_changed_bssid ) ; 
sdata -> u . mgd . flags = 0 ; 
sdata -> vif . bss_conf . mu_mimo_owner = false ; 

if ( status != assoc_rejected ) { 
struct cfg80211_assoc_failure data = { 
. timeout = status == assoc_timeout , 
} ; 
int i ; 

build_bug_on ( array_size ( data . bss ) != 
array_size ( assoc_data -> link ) ) ; 

for ( i = 0 ; i < array_size ( data . bss ) ; i ++ ) 
data . bss [ i ] = assoc_data -> link [ i ] . bss ; 

if ( sdata -> vif . valid_links ) 
data . ap_mld_addr = assoc_data -> ap_addr ; 

cfg80211_assoc_failure ( sdata -> dev , & data ) ; 
} 

mutex_lock ( & sdata -> local -> mtx ) ; 
ieee80211_link_release_channel ( & sdata -> deflink ) ; 
ieee80211_vif_set_links ( sdata , 0 ) ; 
mutex_unlock ( & sdata -> local -> mtx ) ; 
} 

kfree ( assoc_data ) ; 
sdata -> u . mgd . assoc_data = null ; 
} 

static void ieee80211_auth_challenge ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_mgmt * mgmt , size_t len ) 
{ 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_mgd_auth_data * auth_data = sdata -> u . mgd . auth_data ; 
const struct element * challenge ; 
u8 * pos ; 
u32 tx_flags = 0 ; 
struct ieee80211_prep_tx_info info = { 
. subtype = ieee80211_stype_auth , 
} ; 

pos = mgmt -> u . auth . variable ; 
challenge = cfg80211_find_elem ( wlan_eid_challenge , pos , 
len - ( pos - ( u8 * ) mgmt ) ) ; 
if ( ! challenge ) 
return ; 
auth_data -> expected_transaction = 4 ; 
drv_mgd_prepare_tx ( sdata -> local , sdata , & info ) ; 
if ( ieee80211_hw_check ( & local -> hw , reports_tx_ack_status ) ) 
tx_flags = ieee80211_tx_ctl_req_tx_status | 
ieee80211_tx_intfl_mlme_conn_tx ; 
ieee80211_send_auth ( sdata , 3 , auth_data -> algorithm , 0 , 
( void * ) challenge , 
challenge -> datalen + sizeof ( * challenge ) , 
auth_data -> ap_addr , auth_data -> ap_addr , 
auth_data -> key , auth_data -> key_len , 
auth_data -> key_idx , tx_flags ) ; 
} 

static bool ieee80211_mark_sta_auth ( struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
const u8 * ap_addr = ifmgd -> auth_data -> ap_addr ; 
struct sta_info * sta ; 
bool result = true ; 

sdata_info ( sdata , "authenticated\n" ) ; 
ifmgd -> auth_data -> done = true ; 
ifmgd -> auth_data -> timeout = jiffies + ieee80211_auth_wait_assoc ; 
ifmgd -> auth_data -> timeout_started = true ; 
run_again ( sdata , ifmgd -> auth_data -> timeout ) ; 


mutex_lock ( & sdata -> local -> sta_mtx ) ; 
sta = sta_info_get ( sdata , ap_addr ) ; 
if ( ! sta ) { 
warn_once ( 1 , "%s: sta %pm not found" , sdata -> name , ap_addr ) ; 
result = false ; 
goto out ; 
} 
if ( sta_info_move_state ( sta , ieee80211_sta_auth ) ) { 
sdata_info ( sdata , "failed moving %pm to auth\n" , ap_addr ) ; 
result = false ; 
goto out ; 
} 

out : 
mutex_unlock ( & sdata -> local -> sta_mtx ) ; 
return result ; 
} 

static void ieee80211_rx_mgmt_auth ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_mgmt * mgmt , size_t len ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
u16 auth_alg , auth_transaction , status_code ; 
struct ieee80211_event event = { 
. type = mlme_event , 
. u . mlme . data = auth_event , 
} ; 
struct ieee80211_prep_tx_info info = { 
. subtype = ieee80211_stype_auth , 
} ; 

sdata_assert_lock ( sdata ) ; 

if ( len < 24 + 6 ) 
return ; 

if ( ! ifmgd -> auth_data || ifmgd -> auth_data -> done ) 
return ; 

if ( ! ether_addr_equal ( ifmgd -> auth_data -> ap_addr , mgmt -> bssid ) ) 
return ; 

auth_alg = le16_to_cpu ( mgmt -> u . auth . auth_alg ) ; 
auth_transaction = le16_to_cpu ( mgmt -> u . auth . auth_transaction ) ; 
status_code = le16_to_cpu ( mgmt -> u . auth . status_code ) ; 

if ( auth_alg != ifmgd -> auth_data -> algorithm || 
( auth_alg != wlan_auth_sae && 
auth_transaction != ifmgd -> auth_data -> expected_transaction ) || 
( auth_alg == wlan_auth_sae && 
( auth_transaction < ifmgd -> auth_data -> expected_transaction || 
auth_transaction > 2 ) ) ) { 
sdata_info ( sdata , "%pm unexpected authentication state: alg %d (expected %d) transact %d (expected %d)\n" , 
mgmt -> sa , auth_alg , ifmgd -> auth_data -> algorithm , 
auth_transaction , 
ifmgd -> auth_data -> expected_transaction ) ; 
goto notify_driver ; 
} 

if ( status_code != wlan_status_success ) { 
cfg80211_rx_mlme_mgmt ( sdata -> dev , ( u8 * ) mgmt , len ) ; 

if ( auth_alg == wlan_auth_sae && 
( status_code == wlan_status_anti_clog_required || 
( auth_transaction == 1 && 
( status_code == wlan_status_sae_hash_to_element || 
status_code == wlan_status_sae_pk ) ) ) ) { 

ifmgd -> auth_data -> waiting = true ; 
ifmgd -> auth_data -> timeout = 
jiffies + ieee80211_auth_wait_sae_retry ; 
ifmgd -> auth_data -> timeout_started = true ; 
run_again ( sdata , ifmgd -> auth_data -> timeout ) ; 
goto notify_driver ; 
} 

sdata_info ( sdata , "%pm denied authentication (status %d)\n" , 
mgmt -> sa , status_code ) ; 
ieee80211_destroy_auth_data ( sdata , false ) ; 
event . u . mlme . status = mlme_denied ; 
event . u . mlme . reason = status_code ; 
drv_event_callback ( sdata -> local , sdata , & event ) ; 
goto notify_driver ; 
} 

switch ( ifmgd -> auth_data -> algorithm ) { 
case wlan_auth_open : 
case wlan_auth_leap : 
case wlan_auth_ft : 
case wlan_auth_sae : 
case wlan_auth_fils_sk : 
case wlan_auth_fils_sk_pfs : 
case wlan_auth_fils_pk : 
break ; 
case wlan_auth_shared_key : 
if ( ifmgd -> auth_data -> expected_transaction != 4 ) { 
ieee80211_auth_challenge ( sdata , mgmt , len ) ; 

return ; 
} 
break ; 
default : 
warn_once ( 1 , "invalid auth alg %d" , 
ifmgd -> auth_data -> algorithm ) ; 
goto notify_driver ; 
} 

event . u . mlme . status = mlme_success ; 
info . success = 1 ; 
drv_event_callback ( sdata -> local , sdata , & event ) ; 
if ( ifmgd -> auth_data -> algorithm != wlan_auth_sae || 
( auth_transaction == 2 && 
ifmgd -> auth_data -> expected_transaction == 2 ) ) { 
if ( ! ieee80211_mark_sta_auth ( sdata ) ) 
return ; 
} else if ( ifmgd -> auth_data -> algorithm == wlan_auth_sae && 
auth_transaction == 2 ) { 
sdata_info ( sdata , "sae peer confirmed\n" ) ; 
ifmgd -> auth_data -> peer_confirmed = true ; 
} 

cfg80211_rx_mlme_mgmt ( sdata -> dev , ( u8 * ) mgmt , len ) ; 
notify_driver : 
drv_mgd_complete_tx ( sdata -> local , sdata , & info ) ; 
} 




const char * ieee80211_get_reason_code_string ( u16 reason_code ) 
{ 
switch ( reason_code ) { 
case_wlan ( unspecified ) ; 
case_wlan ( prev_auth_not_valid ) ; 
case_wlan ( deauth_leaving ) ; 
case_wlan ( disassoc_due_to_inactivity ) ; 
case_wlan ( disassoc_ap_busy ) ; 
case_wlan ( class2_frame_from_nonauth_sta ) ; 
case_wlan ( class3_frame_from_nonassoc_sta ) ; 
case_wlan ( disassoc_sta_has_left ) ; 
case_wlan ( sta_req_assoc_without_auth ) ; 
case_wlan ( disassoc_bad_power ) ; 
case_wlan ( disassoc_bad_supp_chan ) ; 
case_wlan ( invalid_ie ) ; 
case_wlan ( mic_failure ) ; 
case_wlan ( 4 way_handshake_timeout ) ; 
case_wlan ( group_key_handshake_timeout ) ; 
case_wlan ( ie_different ) ; 
case_wlan ( invalid_group_cipher ) ; 
case_wlan ( invalid_pairwise_cipher ) ; 
case_wlan ( invalid_akmp ) ; 
case_wlan ( unsupp_rsn_version ) ; 
case_wlan ( invalid_rsn_ie_cap ) ; 
case_wlan ( ieee8021x_failed ) ; 
case_wlan ( cipher_suite_rejected ) ; 
case_wlan ( disassoc_unspecified_qos ) ; 
case_wlan ( disassoc_qap_no_bandwidth ) ; 
case_wlan ( disassoc_low_ack ) ; 
case_wlan ( disassoc_qap_exceed_txop ) ; 
case_wlan ( qsta_leave_qbss ) ; 
case_wlan ( qsta_not_use ) ; 
case_wlan ( qsta_require_setup ) ; 
case_wlan ( qsta_timeout ) ; 
case_wlan ( qsta_cipher_not_supp ) ; 
case_wlan ( mesh_peer_canceled ) ; 
case_wlan ( mesh_max_peers ) ; 
case_wlan ( mesh_config ) ; 
case_wlan ( mesh_close ) ; 
case_wlan ( mesh_max_retries ) ; 
case_wlan ( mesh_confirm_timeout ) ; 
case_wlan ( mesh_invalid_gtk ) ; 
case_wlan ( mesh_inconsistent_param ) ; 
case_wlan ( mesh_invalid_security ) ; 
case_wlan ( mesh_path_error ) ; 
case_wlan ( mesh_path_noforward ) ; 
case_wlan ( mesh_path_dest_unreachable ) ; 
case_wlan ( mac_exists_in_mbss ) ; 
case_wlan ( mesh_chan_regulatory ) ; 
case_wlan ( mesh_chan ) ; 
default : return "<unknown>" ; 
} 
} 

static void ieee80211_rx_mgmt_deauth ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_mgmt * mgmt , size_t len ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
u16 reason_code = le16_to_cpu ( mgmt -> u . deauth . reason_code ) ; 

sdata_assert_lock ( sdata ) ; 

if ( len < 24 + 2 ) 
return ; 

if ( ! ether_addr_equal ( mgmt -> bssid , mgmt -> sa ) ) { 
ieee80211_tdls_handle_disconnect ( sdata , mgmt -> sa , reason_code ) ; 
return ; 
} 

if ( ifmgd -> associated && 
ether_addr_equal ( mgmt -> bssid , sdata -> vif . cfg . ap_addr ) ) { 
sdata_info ( sdata , "deauthenticated from %pm (reason: %u=%s)\n" , 
sdata -> vif . cfg . ap_addr , reason_code , 
ieee80211_get_reason_code_string ( reason_code ) ) ; 

ieee80211_set_disassoc ( sdata , 0 , 0 , false , null ) ; 

ieee80211_report_disconnect ( sdata , ( u8 * ) mgmt , len , false , 
reason_code , false ) ; 
return ; 
} 

if ( ifmgd -> assoc_data && 
ether_addr_equal ( mgmt -> bssid , ifmgd -> assoc_data -> ap_addr ) ) { 
sdata_info ( sdata , 
"deauthenticated from %pm while associating (reason: %u=%s)\n" , 
ifmgd -> assoc_data -> ap_addr , reason_code , 
ieee80211_get_reason_code_string ( reason_code ) ) ; 

ieee80211_destroy_assoc_data ( sdata , assoc_abandon ) ; 

cfg80211_rx_mlme_mgmt ( sdata -> dev , ( u8 * ) mgmt , len ) ; 
return ; 
} 
} 


static void ieee80211_rx_mgmt_disassoc ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_mgmt * mgmt , size_t len ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
u16 reason_code ; 

sdata_assert_lock ( sdata ) ; 

if ( len < 24 + 2 ) 
return ; 

if ( ! ifmgd -> associated || 
! ether_addr_equal ( mgmt -> bssid , sdata -> vif . cfg . ap_addr ) ) 
return ; 

reason_code = le16_to_cpu ( mgmt -> u . disassoc . reason_code ) ; 

if ( ! ether_addr_equal ( mgmt -> bssid , mgmt -> sa ) ) { 
ieee80211_tdls_handle_disconnect ( sdata , mgmt -> sa , reason_code ) ; 
return ; 
} 

sdata_info ( sdata , "disassociated from %pm (reason: %u=%s)\n" , 
sdata -> vif . cfg . ap_addr , reason_code , 
ieee80211_get_reason_code_string ( reason_code ) ) ; 

ieee80211_set_disassoc ( sdata , 0 , 0 , false , null ) ; 

ieee80211_report_disconnect ( sdata , ( u8 * ) mgmt , len , false , reason_code , 
false ) ; 
} 

static void ieee80211_get_rates ( struct ieee80211_supported_band * sband , 
u8 * supp_rates , unsigned int supp_rates_len , 
u32 * rates , u32 * basic_rates , 
bool * have_higher_than_11mbit , 
int * min_rate , int * min_rate_index , 
int shift ) 
{ 
int i , j ; 

for ( i = 0 ; i < supp_rates_len ; i ++ ) { 
int rate = supp_rates [ i ] & 0x7f ; 
bool is_basic = ! ! ( supp_rates [ i ] & 0x80 ) ; 

if ( ( rate * 5 * ( 1 << shift ) ) > 110 ) 
* have_higher_than_11mbit = true ; 









if ( supp_rates [ i ] == ( 0x80 | bss_membership_selector_ht_phy ) || 
supp_rates [ i ] == ( 0x80 | bss_membership_selector_vht_phy ) || 
supp_rates [ i ] == ( 0x80 | bss_membership_selector_he_phy ) || 
supp_rates [ i ] == ( 0x80 | bss_membership_selector_sae_h2e ) ) 
continue ; 

for ( j = 0 ; j < sband -> n_bitrates ; j ++ ) { 
struct ieee80211_rate * br ; 
int brate ; 

br = & sband -> bitrates [ j ] ; 

brate = div_round_up ( br -> bitrate , ( 1 << shift ) * 5 ) ; 
if ( brate == rate ) { 
* rates |= bit ( j ) ; 
if ( is_basic ) 
* basic_rates |= bit ( j ) ; 
if ( ( rate * 5 ) < * min_rate ) { 
* min_rate = rate * 5 ; 
* min_rate_index = j ; 
} 
break ; 
} 
} 
} 
} 

static bool ieee80211_twt_req_supported ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_supported_band * sband , 
const struct link_sta_info * link_sta , 
const struct ieee802_11_elems * elems ) 
{ 
const struct ieee80211_sta_he_cap * own_he_cap = 
ieee80211_get_he_iftype_cap ( sband , 
ieee80211_vif_type_p2p ( & sdata -> vif ) ) ; 

if ( elems -> ext_capab_len < 10 ) 
return false ; 

if ( ! ( elems -> ext_capab [ 9 ] & wlan_ext_capa10_twt_responder_support ) ) 
return false ; 

return link_sta -> pub -> he_cap . he_cap_elem . mac_cap_info [ 0 ] & 
ieee80211_he_mac_cap0_twt_res && 
own_he_cap && 
( own_he_cap -> he_cap_elem . mac_cap_info [ 0 ] & 
ieee80211_he_mac_cap0_twt_req ) ; 
} 

static int ieee80211_recalc_twt_req ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_supported_band * sband , 
struct ieee80211_link_data * link , 
struct link_sta_info * link_sta , 
struct ieee802_11_elems * elems ) 
{ 
bool twt = ieee80211_twt_req_supported ( sdata , sband , link_sta , elems ) ; 

if ( link -> conf -> twt_requester != twt ) { 
link -> conf -> twt_requester = twt ; 
return bss_changed_twt ; 
} 
return 0 ; 
} 

static bool ieee80211_twt_bcast_support ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_bss_conf * bss_conf , 
struct ieee80211_supported_band * sband , 
struct link_sta_info * link_sta ) 
{ 
const struct ieee80211_sta_he_cap * own_he_cap = 
ieee80211_get_he_iftype_cap ( sband , 
ieee80211_vif_type_p2p ( & sdata -> vif ) ) ; 

return bss_conf -> he_support && 
( link_sta -> pub -> he_cap . he_cap_elem . mac_cap_info [ 2 ] & 
ieee80211_he_mac_cap2_bcast_twt ) && 
own_he_cap && 
( own_he_cap -> he_cap_elem . mac_cap_info [ 2 ] & 
ieee80211_he_mac_cap2_bcast_twt ) ; 
} 

static bool ieee80211_assoc_config_link ( struct ieee80211_link_data * link , 
struct link_sta_info * link_sta , 
struct cfg80211_bss * cbss , 
struct ieee80211_mgmt * mgmt , 
const u8 * elem_start , 
unsigned int elem_len , 
u64 * changed ) 
{ 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 
struct ieee80211_mgd_assoc_data * assoc_data = sdata -> u . mgd . assoc_data ; 
struct ieee80211_bss_conf * bss_conf = link -> conf ; 
struct ieee80211_local * local = sdata -> local ; 
unsigned int link_id = link -> link_id ; 
struct ieee80211_elems_parse_params parse_params = { 
. start = elem_start , 
. len = elem_len , 
. link_id = link_id == assoc_data -> assoc_link_id ? - 1 : link_id , 
. from_ap = true , 
} ; 
bool is_6ghz = cbss -> channel -> band == nl80211_band_6ghz ; 
bool is_s1g = cbss -> channel -> band == nl80211_band_s1ghz ; 
const struct cfg80211_bss_ies * bss_ies = null ; 
struct ieee80211_supported_band * sband ; 
struct ieee802_11_elems * elems ; 
u16 capab_info ; 
bool ret ; 

elems = ieee802_11_parse_elems_full ( & parse_params ) ; 
if ( ! elems ) 
return false ; 

if ( link_id == assoc_data -> assoc_link_id ) { 
capab_info = le16_to_cpu ( mgmt -> u . assoc_resp . capab_info ) ; 





assoc_data -> link [ link_id ] . status = wlan_status_success ; 
} else if ( ! elems -> prof ) { 
ret = false ; 
goto out ; 
} else { 
const u8 * ptr = elems -> prof -> variable + 
elems -> prof -> sta_info_len - 1 ; 





capab_info = get_unaligned_le16 ( ptr ) ; 
assoc_data -> link [ link_id ] . status = get_unaligned_le16 ( ptr + 2 ) ; 

if ( assoc_data -> link [ link_id ] . status != wlan_status_success ) { 
link_info ( link , "association response status code=%u\n" , 
assoc_data -> link [ link_id ] . status ) ; 
ret = true ; 
goto out ; 
} 
} 

if ( ! is_s1g && ! elems -> supp_rates ) { 
sdata_info ( sdata , "no supprates element in assocresp\n" ) ; 
ret = false ; 
goto out ; 
} 

link -> u . mgd . tdls_chan_switch_prohibited = 
elems -> ext_capab && elems -> ext_capab_len >= 5 && 
( elems -> ext_capab [ 4 ] & wlan_ext_capa5_tdls_ch_sw_prohibited ) ; 








if ( ! is_6ghz && 
( ( assoc_data -> wmm && ! elems -> wmm_param ) || 
( ! ( link -> u . mgd . conn_flags & ieee80211_conn_disable_ht ) && 
( ! elems -> ht_cap_elem || ! elems -> ht_operation ) ) || 
( ! ( link -> u . mgd . conn_flags & ieee80211_conn_disable_vht ) && 
( ! elems -> vht_cap_elem || ! elems -> vht_operation ) ) ) ) { 
const struct cfg80211_bss_ies * ies ; 
struct ieee802_11_elems * bss_elems ; 

rcu_read_lock ( ) ; 
ies = rcu_dereference ( cbss -> ies ) ; 
if ( ies ) 
bss_ies = kmemdup ( ies , sizeof ( * ies ) + ies -> len , 
gfp_atomic ) ; 
rcu_read_unlock ( ) ; 
if ( ! bss_ies ) { 
ret = false ; 
goto out ; 
} 

parse_params . start = bss_ies -> data ; 
parse_params . len = bss_ies -> len ; 
parse_params . bss = cbss ; 
bss_elems = ieee802_11_parse_elems_full ( & parse_params ) ; 
if ( ! bss_elems ) { 
ret = false ; 
goto out ; 
} 

if ( assoc_data -> wmm && 
! elems -> wmm_param && bss_elems -> wmm_param ) { 
elems -> wmm_param = bss_elems -> wmm_param ; 
sdata_info ( sdata , 
"ap bug: wmm param missing from assocresp\n" ) ; 
} 





if ( ! elems -> ht_cap_elem && bss_elems -> ht_cap_elem && 
! ( link -> u . mgd . conn_flags & ieee80211_conn_disable_ht ) ) { 
elems -> ht_cap_elem = bss_elems -> ht_cap_elem ; 
sdata_info ( sdata , 
"ap bug: ht capability missing from assocresp\n" ) ; 
} 
if ( ! elems -> ht_operation && bss_elems -> ht_operation && 
! ( link -> u . mgd . conn_flags & ieee80211_conn_disable_ht ) ) { 
elems -> ht_operation = bss_elems -> ht_operation ; 
sdata_info ( sdata , 
"ap bug: ht operation missing from assocresp\n" ) ; 
} 
if ( ! elems -> vht_cap_elem && bss_elems -> vht_cap_elem && 
! ( link -> u . mgd . conn_flags & ieee80211_conn_disable_vht ) ) { 
elems -> vht_cap_elem = bss_elems -> vht_cap_elem ; 
sdata_info ( sdata , 
"ap bug: vht capa missing from assocresp\n" ) ; 
} 
if ( ! elems -> vht_operation && bss_elems -> vht_operation && 
! ( link -> u . mgd . conn_flags & ieee80211_conn_disable_vht ) ) { 
elems -> vht_operation = bss_elems -> vht_operation ; 
sdata_info ( sdata , 
"ap bug: vht operation missing from assocresp\n" ) ; 
} 

kfree ( bss_elems ) ; 
} 





if ( ! is_6ghz && ! ( link -> u . mgd . conn_flags & ieee80211_conn_disable_ht ) && 
( ! elems -> wmm_param || ! elems -> ht_cap_elem || ! elems -> ht_operation ) ) { 
sdata_info ( sdata , 
"ht ap is missing wmm params or ht capability/operation\n" ) ; 
ret = false ; 
goto out ; 
} 

if ( ! is_6ghz && ! ( link -> u . mgd . conn_flags & ieee80211_conn_disable_vht ) && 
( ! elems -> vht_cap_elem || ! elems -> vht_operation ) ) { 
sdata_info ( sdata , 
"vht ap is missing vht capability/operation\n" ) ; 
ret = false ; 
goto out ; 
} 

if ( is_6ghz && ! ( link -> u . mgd . conn_flags & ieee80211_conn_disable_he ) && 
! elems -> he_6ghz_capa ) { 
sdata_info ( sdata , 
"he 6 ghz ap is missing he 6 ghz band capability\n" ) ; 
ret = false ; 
goto out ; 
} 

if ( warn_on ( ! link -> conf -> chandef . chan ) ) { 
ret = false ; 
goto out ; 
} 
sband = local -> hw . wiphy -> bands [ link -> conf -> chandef . chan -> band ] ; 

if ( ! ( link -> u . mgd . conn_flags & ieee80211_conn_disable_he ) && 
( ! elems -> he_cap || ! elems -> he_operation ) ) { 
sdata_info ( sdata , 
"he ap is missing he capability/operation\n" ) ; 
ret = false ; 
goto out ; 
} 


if ( elems -> ht_cap_elem && ! ( link -> u . mgd . conn_flags & ieee80211_conn_disable_ht ) ) 
ieee80211_ht_cap_ie_to_sta_ht_cap ( sdata , sband , 
elems -> ht_cap_elem , 
link_sta ) ; 

if ( elems -> vht_cap_elem && ! ( link -> u . mgd . conn_flags & ieee80211_conn_disable_vht ) ) 
ieee80211_vht_cap_ie_to_sta_vht_cap ( sdata , sband , 
elems -> vht_cap_elem , 
link_sta ) ; 

if ( elems -> he_operation && ! ( link -> u . mgd . conn_flags & ieee80211_conn_disable_he ) && 
elems -> he_cap ) { 
ieee80211_he_cap_ie_to_sta_he_cap ( sdata , sband , 
elems -> he_cap , 
elems -> he_cap_len , 
elems -> he_6ghz_capa , 
link_sta ) ; 

bss_conf -> he_support = link_sta -> pub -> he_cap . has_he ; 
if ( elems -> rsnx && elems -> rsnx_len && 
( elems -> rsnx [ 0 ] & wlan_rsnx_capa_protected_twt ) && 
wiphy_ext_feature_isset ( local -> hw . wiphy , 
nl80211_ext_feature_protected_twt ) ) 
bss_conf -> twt_protected = true ; 
else 
bss_conf -> twt_protected = false ; 

* changed |= ieee80211_recalc_twt_req ( sdata , sband , link , 
link_sta , elems ) ; 

if ( elems -> eht_operation && elems -> eht_cap && 
! ( link -> u . mgd . conn_flags & ieee80211_conn_disable_eht ) ) { 
ieee80211_eht_cap_ie_to_sta_eht_cap ( sdata , sband , 
elems -> he_cap , 
elems -> he_cap_len , 
elems -> eht_cap , 
elems -> eht_cap_len , 
link_sta ) ; 

bss_conf -> eht_support = link_sta -> pub -> eht_cap . has_eht ; 
* changed |= bss_changed_eht_puncturing ; 
} else { 
bss_conf -> eht_support = false ; 
} 
} else { 
bss_conf -> he_support = false ; 
bss_conf -> twt_requester = false ; 
bss_conf -> twt_protected = false ; 
bss_conf -> eht_support = false ; 
} 

bss_conf -> twt_broadcast = 
ieee80211_twt_bcast_support ( sdata , bss_conf , sband , link_sta ) ; 

if ( bss_conf -> he_support ) { 
bss_conf -> he_bss_color . color = 
le32_get_bits ( elems -> he_operation -> he_oper_params , 
ieee80211_he_operation_bss_color_mask ) ; 
bss_conf -> he_bss_color . partial = 
le32_get_bits ( elems -> he_operation -> he_oper_params , 
ieee80211_he_operation_partial_bss_color ) ; 
bss_conf -> he_bss_color . enabled = 
! le32_get_bits ( elems -> he_operation -> he_oper_params , 
ieee80211_he_operation_bss_color_disabled ) ; 

if ( bss_conf -> he_bss_color . enabled ) 
* changed |= bss_changed_he_bss_color ; 

bss_conf -> htc_trig_based_pkt_ext = 
le32_get_bits ( elems -> he_operation -> he_oper_params , 
ieee80211_he_operation_dflt_pe_duration_mask ) ; 
bss_conf -> frame_time_rts_th = 
le32_get_bits ( elems -> he_operation -> he_oper_params , 
ieee80211_he_operation_rts_threshold_mask ) ; 

bss_conf -> uora_exists = ! ! elems -> uora_element ; 
if ( elems -> uora_element ) 
bss_conf -> uora_ocw_range = elems -> uora_element [ 0 ] ; 

ieee80211_he_op_ie_to_bss_conf ( & sdata -> vif , elems -> he_operation ) ; 
ieee80211_he_spr_ie_to_bss_conf ( & sdata -> vif , elems -> he_spr ) ; 

} 

if ( cbss -> transmitted_bss ) { 
bss_conf -> nontransmitted = true ; 
ether_addr_copy ( bss_conf -> transmitter_bssid , 
cbss -> transmitted_bss -> bssid ) ; 
bss_conf -> bssid_indicator = cbss -> max_bssid_indicator ; 
bss_conf -> bssid_index = cbss -> bssid_index ; 
} 













if ( elems -> opmode_notif && 
! ( * elems -> opmode_notif & ieee80211_opmode_notif_rx_nss_type_bf ) ) { 
u8 nss ; 

nss = * elems -> opmode_notif & ieee80211_opmode_notif_rx_nss_mask ; 
nss >>= ieee80211_opmode_notif_rx_nss_shift ; 
nss += 1 ; 
link_sta -> pub -> rx_nss = nss ; 
} 







link -> u . mgd . wmm_last_param_set = - 1 ; 
link -> u . mgd . mu_edca_last_param_set = - 1 ; 

if ( link -> u . mgd . disable_wmm_tracking ) { 
ieee80211_set_wmm_default ( link , false , false ) ; 
} else if ( ! ieee80211_sta_wmm_params ( local , link , elems -> wmm_param , 
elems -> wmm_param_len , 
elems -> mu_edca_param_set ) ) { 

ieee80211_set_wmm_default ( link , false , true ) ; 







link -> u . mgd . disable_wmm_tracking = true ; 
} 

if ( elems -> max_idle_period_ie ) { 
bss_conf -> max_idle_period = 
le16_to_cpu ( elems -> max_idle_period_ie -> max_idle_period ) ; 
bss_conf -> protected_keep_alive = 
! ! ( elems -> max_idle_period_ie -> idle_options & 
wlan_idle_options_protected_keep_alive ) ; 
* changed |= bss_changed_keep_alive ; 
} else { 
bss_conf -> max_idle_period = 0 ; 
bss_conf -> protected_keep_alive = false ; 
} 



bss_conf -> assoc_capability = capab_info ; 

ret = true ; 
out : 
kfree ( elems ) ; 
kfree ( bss_ies ) ; 
return ret ; 
} 

static int ieee80211_mgd_setup_link_sta ( struct ieee80211_link_data * link , 
struct sta_info * sta , 
struct link_sta_info * link_sta , 
struct cfg80211_bss * cbss ) 
{ 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_bss * bss = ( void * ) cbss -> priv ; 
u32 rates = 0 , basic_rates = 0 ; 
bool have_higher_than_11mbit = false ; 
int min_rate = int_max , min_rate_index = - 1 ; 

int shift = ieee80211_vif_get_shift ( & sdata -> vif ) ; 
struct ieee80211_supported_band * sband ; 

memcpy ( link_sta -> addr , cbss -> bssid , eth_alen ) ; 
memcpy ( link_sta -> pub -> addr , cbss -> bssid , eth_alen ) ; 


if ( cbss -> channel -> band == nl80211_band_s1ghz ) { 
ieee80211_s1g_sta_rate_init ( sta ) ; 
return 0 ; 
} 

sband = local -> hw . wiphy -> bands [ cbss -> channel -> band ] ; 

ieee80211_get_rates ( sband , bss -> supp_rates , bss -> supp_rates_len , 
& rates , & basic_rates , & have_higher_than_11mbit , 
& min_rate , & min_rate_index , shift ) ; 











if ( min_rate_index < 0 ) { 
link_info ( link , "no legacy rates in association response\n" ) ; 
return - einval ; 
} else if ( ! basic_rates ) { 
link_info ( link , "no basic rates, using min rate instead\n" ) ; 
basic_rates = bit ( min_rate_index ) ; 
} 

if ( rates ) 
link_sta -> pub -> supp_rates [ cbss -> channel -> band ] = rates ; 
else 
link_info ( link , "no rates found, keeping mandatory only\n" ) ; 

link -> conf -> basic_rates = basic_rates ; 


link -> operating_11g_mode = sband -> band == nl80211_band_2ghz && 
have_higher_than_11mbit ; 

return 0 ; 
} 

static u8 ieee80211_max_rx_chains ( struct ieee80211_link_data * link , 
struct cfg80211_bss * cbss ) 
{ 
struct ieee80211_he_mcs_nss_supp * he_mcs_nss_supp ; 
const struct element * ht_cap_elem , * vht_cap_elem ; 
const struct cfg80211_bss_ies * ies ; 
const struct ieee80211_ht_cap * ht_cap ; 
const struct ieee80211_vht_cap * vht_cap ; 
const struct ieee80211_he_cap_elem * he_cap ; 
const struct element * he_cap_elem ; 
u16 mcs_80_map , mcs_160_map ; 
int i , mcs_nss_size ; 
bool support_160 ; 
u8 chains = 1 ; 

if ( link -> u . mgd . conn_flags & ieee80211_conn_disable_ht ) 
return chains ; 

ht_cap_elem = ieee80211_bss_get_elem ( cbss , wlan_eid_ht_capability ) ; 
if ( ht_cap_elem && ht_cap_elem -> datalen >= sizeof ( * ht_cap ) ) { 
ht_cap = ( void * ) ht_cap_elem -> data ; 
chains = ieee80211_mcs_to_chains ( & ht_cap -> mcs ) ; 




} 

if ( link -> u . mgd . conn_flags & ieee80211_conn_disable_vht ) 
return chains ; 

vht_cap_elem = ieee80211_bss_get_elem ( cbss , wlan_eid_vht_capability ) ; 
if ( vht_cap_elem && vht_cap_elem -> datalen >= sizeof ( * vht_cap ) ) { 
u8 nss ; 
u16 tx_mcs_map ; 

vht_cap = ( void * ) vht_cap_elem -> data ; 
tx_mcs_map = le16_to_cpu ( vht_cap -> supp_mcs . tx_mcs_map ) ; 
for ( nss = 8 ; nss > 0 ; nss -- ) { 
if ( ( ( tx_mcs_map >> ( 2 * ( nss - 1 ) ) ) & 3 ) != 
ieee80211_vht_mcs_not_supported ) 
break ; 
} 

chains = max ( chains , nss ) ; 
} 

if ( link -> u . mgd . conn_flags & ieee80211_conn_disable_he ) 
return chains ; 

ies = rcu_dereference ( cbss -> ies ) ; 
he_cap_elem = cfg80211_find_ext_elem ( wlan_eid_ext_he_capability , 
ies -> data , ies -> len ) ; 

if ( ! he_cap_elem || he_cap_elem -> datalen < sizeof ( * he_cap ) ) 
return chains ; 


he_cap = ( void * ) ( he_cap_elem -> data + 1 ) ; 
mcs_nss_size = ieee80211_he_mcs_nss_size ( he_cap ) ; 


if ( he_cap_elem -> datalen < 1 + mcs_nss_size + sizeof ( * he_cap ) ) 
return chains ; 


he_mcs_nss_supp = ( void * ) ( he_cap + 1 ) ; 

mcs_80_map = le16_to_cpu ( he_mcs_nss_supp -> tx_mcs_80 ) ; 

for ( i = 7 ; i >= 0 ; i -- ) { 
u8 mcs_80 = mcs_80_map >> ( 2 * i ) & 3 ; 

if ( mcs_80 != ieee80211_vht_mcs_not_supported ) { 
chains = max_t ( u8 , chains , i + 1 ) ; 
break ; 
} 
} 

support_160 = he_cap -> phy_cap_info [ 0 ] & 
ieee80211_he_phy_cap0_channel_width_set_160mhz_in_5g ; 

if ( ! support_160 ) 
return chains ; 

mcs_160_map = le16_to_cpu ( he_mcs_nss_supp -> tx_mcs_160 ) ; 
for ( i = 7 ; i >= 0 ; i -- ) { 
u8 mcs_160 = mcs_160_map >> ( 2 * i ) & 3 ; 

if ( mcs_160 != ieee80211_vht_mcs_not_supported ) { 
chains = max_t ( u8 , chains , i + 1 ) ; 
break ; 
} 
} 

return chains ; 
} 

static bool 
ieee80211_verify_peer_he_mcs_support ( struct ieee80211_sub_if_data * sdata , 
const struct cfg80211_bss_ies * ies , 
const struct ieee80211_he_operation * he_op ) 
{ 
const struct element * he_cap_elem ; 
const struct ieee80211_he_cap_elem * he_cap ; 
struct ieee80211_he_mcs_nss_supp * he_mcs_nss_supp ; 
u16 mcs_80_map_tx , mcs_80_map_rx ; 
u16 ap_min_req_set ; 
int mcs_nss_size ; 
int nss ; 

he_cap_elem = cfg80211_find_ext_elem ( wlan_eid_ext_he_capability , 
ies -> data , ies -> len ) ; 

if ( ! he_cap_elem ) 
return false ; 


if ( he_cap_elem -> datalen < 1 + sizeof ( * he_cap ) ) { 
sdata_info ( sdata , 
"invalid he elem, disable he\n" ) ; 
return false ; 
} 


he_cap = ( void * ) ( he_cap_elem -> data + 1 ) ; 
mcs_nss_size = ieee80211_he_mcs_nss_size ( he_cap ) ; 


if ( he_cap_elem -> datalen < 1 + sizeof ( * he_cap ) + mcs_nss_size ) { 
sdata_info ( sdata , 
"invalid he elem with nss size, disable he\n" ) ; 
return false ; 
} 


he_mcs_nss_supp = ( void * ) ( he_cap + 1 ) ; 

mcs_80_map_tx = le16_to_cpu ( he_mcs_nss_supp -> tx_mcs_80 ) ; 
mcs_80_map_rx = le16_to_cpu ( he_mcs_nss_supp -> rx_mcs_80 ) ; 









if ( ( mcs_80_map_tx & 0x3 ) == ieee80211_he_mcs_not_supported || 
( mcs_80_map_rx & 0x3 ) == ieee80211_he_mcs_not_supported ) { 
sdata_info ( sdata , 
"missing mandatory rates for 1 nss, rx 0x%x, tx 0x%x, disable he\n" , 
mcs_80_map_tx , mcs_80_map_rx ) ; 
return false ; 
} 

if ( ! he_op ) 
return true ; 

ap_min_req_set = le16_to_cpu ( he_op -> he_mcs_nss_set ) ; 






if ( ! ap_min_req_set ) 
return true ; 














for ( nss = 8 ; nss > 0 ; nss -- ) { 
u8 ap_op_val = ( ap_min_req_set >> ( 2 * ( nss - 1 ) ) ) & 3 ; 
u8 ap_rx_val ; 
u8 ap_tx_val ; 

if ( ap_op_val == ieee80211_he_mcs_not_supported ) 
continue ; 

ap_rx_val = ( mcs_80_map_rx >> ( 2 * ( nss - 1 ) ) ) & 3 ; 
ap_tx_val = ( mcs_80_map_tx >> ( 2 * ( nss - 1 ) ) ) & 3 ; 

if ( ap_rx_val == ieee80211_he_mcs_not_supported || 
ap_tx_val == ieee80211_he_mcs_not_supported || 
ap_rx_val < ap_op_val || ap_tx_val < ap_op_val ) { 
sdata_info ( sdata , 
"invalid rates for %d nss, rx %d, tx %d oper %d, disable he\n" , 
nss , ap_rx_val , ap_rx_val , ap_op_val ) ; 
return false ; 
} 
} 

return true ; 
} 

static bool 
ieee80211_verify_sta_he_mcs_support ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_supported_band * sband , 
const struct ieee80211_he_operation * he_op ) 
{ 
const struct ieee80211_sta_he_cap * sta_he_cap = 
ieee80211_get_he_iftype_cap ( sband , 
ieee80211_vif_type_p2p ( & sdata -> vif ) ) ; 
u16 ap_min_req_set ; 
int i ; 

if ( ! sta_he_cap || ! he_op ) 
return false ; 

ap_min_req_set = le16_to_cpu ( he_op -> he_mcs_nss_set ) ; 






if ( ! ap_min_req_set ) 
return true ; 


for ( i = 0 ; i < 3 ; i ++ ) { 
const struct ieee80211_he_mcs_nss_supp * sta_mcs_nss_supp = 
& sta_he_cap -> he_mcs_nss_supp ; 
u16 sta_mcs_map_rx = 
le16_to_cpu ( ( ( __le16 * ) sta_mcs_nss_supp ) [ 2 * i ] ) ; 
u16 sta_mcs_map_tx = 
le16_to_cpu ( ( ( __le16 * ) sta_mcs_nss_supp ) [ 2 * i + 1 ] ) ; 
u8 nss ; 
bool verified = true ; 










for ( nss = 8 ; nss > 0 ; nss -- ) { 
u8 sta_rx_val = ( sta_mcs_map_rx >> ( 2 * ( nss - 1 ) ) ) & 3 ; 
u8 sta_tx_val = ( sta_mcs_map_tx >> ( 2 * ( nss - 1 ) ) ) & 3 ; 
u8 ap_val = ( ap_min_req_set >> ( 2 * ( nss - 1 ) ) ) & 3 ; 

if ( ap_val == ieee80211_he_mcs_not_supported ) 
continue ; 













if ( sta_rx_val == ieee80211_he_mcs_not_supported || 
sta_tx_val == ieee80211_he_mcs_not_supported || 
( ap_val > sta_rx_val ) || ( ap_val > sta_tx_val ) ) { 
verified = false ; 
break ; 
} 
} 

if ( verified ) 
return true ; 
} 


return false ; 
} 

static int ieee80211_prep_channel ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_link_data * link , 
struct cfg80211_bss * cbss , 
ieee80211_conn_flags_t * conn_flags ) 
{ 
struct ieee80211_local * local = sdata -> local ; 
const struct ieee80211_ht_cap * ht_cap = null ; 
const struct ieee80211_ht_operation * ht_oper = null ; 
const struct ieee80211_vht_operation * vht_oper = null ; 
const struct ieee80211_he_operation * he_oper = null ; 
const struct ieee80211_eht_operation * eht_oper = null ; 
const struct ieee80211_s1g_oper_ie * s1g_oper = null ; 
struct ieee80211_supported_band * sband ; 
struct cfg80211_chan_def chandef ; 
bool is_6ghz = cbss -> channel -> band == nl80211_band_6ghz ; 
bool is_5ghz = cbss -> channel -> band == nl80211_band_5ghz ; 
struct ieee80211_bss * bss = ( void * ) cbss -> priv ; 
struct ieee80211_elems_parse_params parse_params = { 
. bss = cbss , 
. link_id = - 1 , 
. from_ap = true , 
} ; 
struct ieee802_11_elems * elems ; 
const struct cfg80211_bss_ies * ies ; 
int ret ; 
u32 i ; 
bool have_80mhz ; 

rcu_read_lock ( ) ; 

ies = rcu_dereference ( cbss -> ies ) ; 
parse_params . start = ies -> data ; 
parse_params . len = ies -> len ; 
elems = ieee802_11_parse_elems_full ( & parse_params ) ; 
if ( ! elems ) { 
rcu_read_unlock ( ) ; 
return - enomem ; 
} 

sband = local -> hw . wiphy -> bands [ cbss -> channel -> band ] ; 

* conn_flags &= ~ ( ieee80211_conn_disable_40mhz | 
ieee80211_conn_disable_80p80mhz | 
ieee80211_conn_disable_160mhz ) ; 


if ( ! sband -> ht_cap . ht_supported && ! is_6ghz ) { 
mlme_dbg ( sdata , "ht not supported, disabling ht/vht/he/eht\n" ) ; 
* conn_flags |= ieee80211_conn_disable_ht ; 
* conn_flags |= ieee80211_conn_disable_vht ; 
* conn_flags |= ieee80211_conn_disable_he ; 
* conn_flags |= ieee80211_conn_disable_eht ; 
} 

if ( ! sband -> vht_cap . vht_supported && is_5ghz ) { 
mlme_dbg ( sdata , "vht not supported, disabling vht/he/eht\n" ) ; 
* conn_flags |= ieee80211_conn_disable_vht ; 
* conn_flags |= ieee80211_conn_disable_he ; 
* conn_flags |= ieee80211_conn_disable_eht ; 
} 

if ( ! ieee80211_get_he_iftype_cap ( sband , 
ieee80211_vif_type_p2p ( & sdata -> vif ) ) ) { 
mlme_dbg ( sdata , "he not supported, disabling he and eht\n" ) ; 
* conn_flags |= ieee80211_conn_disable_he ; 
* conn_flags |= ieee80211_conn_disable_eht ; 
} 

if ( ! ieee80211_get_eht_iftype_cap ( sband , 
ieee80211_vif_type_p2p ( & sdata -> vif ) ) ) { 
mlme_dbg ( sdata , "eht not supported, disabling eht\n" ) ; 
* conn_flags |= ieee80211_conn_disable_eht ; 
} 

if ( ! ( * conn_flags & ieee80211_conn_disable_ht ) && ! is_6ghz ) { 
ht_oper = elems -> ht_operation ; 
ht_cap = elems -> ht_cap_elem ; 

if ( ! ht_cap ) { 
* conn_flags |= ieee80211_conn_disable_ht ; 
ht_oper = null ; 
} 
} 

if ( ! ( * conn_flags & ieee80211_conn_disable_vht ) && ! is_6ghz ) { 
vht_oper = elems -> vht_operation ; 
if ( vht_oper && ! ht_oper ) { 
vht_oper = null ; 
sdata_info ( sdata , 
"ap advertised vht without ht, disabling ht/vht/he\n" ) ; 
* conn_flags |= ieee80211_conn_disable_ht ; 
* conn_flags |= ieee80211_conn_disable_vht ; 
* conn_flags |= ieee80211_conn_disable_he ; 
* conn_flags |= ieee80211_conn_disable_eht ; 
} 

if ( ! elems -> vht_cap_elem ) { 
* conn_flags |= ieee80211_conn_disable_vht ; 
vht_oper = null ; 
} 
} 

if ( ! ( * conn_flags & ieee80211_conn_disable_he ) ) { 
he_oper = elems -> he_operation ; 

if ( link && is_6ghz ) { 
struct ieee80211_bss_conf * bss_conf ; 
u8 j = 0 ; 

bss_conf = link -> conf ; 

if ( elems -> pwr_constr_elem ) 
bss_conf -> pwr_reduction = * elems -> pwr_constr_elem ; 

build_bug_on ( array_size ( bss_conf -> tx_pwr_env ) != 
array_size ( elems -> tx_pwr_env ) ) ; 

for ( i = 0 ; i < elems -> tx_pwr_env_num ; i ++ ) { 
if ( elems -> tx_pwr_env_len [ i ] > 
sizeof ( bss_conf -> tx_pwr_env [ j ] ) ) 
continue ; 

bss_conf -> tx_pwr_env_num ++ ; 
memcpy ( & bss_conf -> tx_pwr_env [ j ] , elems -> tx_pwr_env [ i ] , 
elems -> tx_pwr_env_len [ i ] ) ; 
j ++ ; 
} 
} 

if ( ! ieee80211_verify_peer_he_mcs_support ( sdata , ies , he_oper ) || 
! ieee80211_verify_sta_he_mcs_support ( sdata , sband , he_oper ) ) 
* conn_flags |= ieee80211_conn_disable_he | 
ieee80211_conn_disable_eht ; 
} 







if ( ! ( * conn_flags & 
( ieee80211_conn_disable_he | 
ieee80211_conn_disable_eht ) ) && 
he_oper ) { 
const struct cfg80211_bss_ies * cbss_ies ; 
const u8 * eht_oper_ie ; 

cbss_ies = rcu_dereference ( cbss -> ies ) ; 
eht_oper_ie = cfg80211_find_ext_ie ( wlan_eid_ext_eht_operation , 
cbss_ies -> data , cbss_ies -> len ) ; 
if ( eht_oper_ie && eht_oper_ie [ 1 ] >= 
1 + sizeof ( struct ieee80211_eht_operation ) ) 
eht_oper = ( void * ) ( eht_oper_ie + 3 ) ; 
else 
eht_oper = null ; 
} 


have_80mhz = false ; 
for ( i = 0 ; i < sband -> n_channels ; i ++ ) { 
if ( sband -> channels [ i ] . flags & ( ieee80211_chan_disabled | 
ieee80211_chan_no_80mhz ) ) 
continue ; 

have_80mhz = true ; 
break ; 
} 

if ( ! have_80mhz ) { 
sdata_info ( sdata , "80 mhz not supported, disabling vht\n" ) ; 
* conn_flags |= ieee80211_conn_disable_vht ; 
} 

if ( sband -> band == nl80211_band_s1ghz ) { 
s1g_oper = elems -> s1g_oper ; 
if ( ! s1g_oper ) 
sdata_info ( sdata , 
"ap missing s1g operation element?\n" ) ; 
} 

* conn_flags |= 
ieee80211_determine_chantype ( sdata , link , * conn_flags , 
sband , 
cbss -> channel , 
bss -> vht_cap_info , 
ht_oper , vht_oper , 
he_oper , eht_oper , 
s1g_oper , 
& chandef , false ) ; 

if ( link ) 
link -> needed_rx_chains = 
min ( ieee80211_max_rx_chains ( link , cbss ) , 
local -> rx_chains ) ; 

rcu_read_unlock ( ) ; 

kfree ( elems ) ; 
elems = null ; 

if ( * conn_flags & ieee80211_conn_disable_he && is_6ghz ) { 
sdata_info ( sdata , "rejecting non-he 6/7 ghz connection" ) ; 
return - einval ; 
} 

if ( ! link ) 
return 0 ; 


link -> smps_mode = ieee80211_smps_off ; 

mutex_lock ( & local -> mtx ) ; 





ret = ieee80211_link_use_channel ( link , & chandef , 
ieee80211_chanctx_shared ) ; 


if ( chandef . width == nl80211_chan_width_5 || 
chandef . width == nl80211_chan_width_10 ) 
goto out ; 

while ( ret && chandef . width != nl80211_chan_width_20_noht ) { 
* conn_flags |= 
ieee80211_chandef_downgrade ( & chandef ) ; 
ret = ieee80211_link_use_channel ( link , & chandef , 
ieee80211_chanctx_shared ) ; 
} 
out : 
mutex_unlock ( & local -> mtx ) ; 
return ret ; 
} 

static bool ieee80211_get_dtim ( const struct cfg80211_bss_ies * ies , 
u8 * dtim_count , u8 * dtim_period ) 
{ 
const u8 * tim_ie = cfg80211_find_ie ( wlan_eid_tim , ies -> data , ies -> len ) ; 
const u8 * idx_ie = cfg80211_find_ie ( wlan_eid_multi_bssid_idx , ies -> data , 
ies -> len ) ; 
const struct ieee80211_tim_ie * tim = null ; 
const struct ieee80211_bssid_index * idx ; 
bool valid = tim_ie && tim_ie [ 1 ] >= 2 ; 

if ( valid ) 
tim = ( void * ) ( tim_ie + 2 ) ; 

if ( dtim_count ) 
* dtim_count = valid ? tim -> dtim_count : 0 ; 

if ( dtim_period ) 
* dtim_period = valid ? tim -> dtim_period : 0 ; 


if ( ! idx_ie || idx_ie [ 1 ] < 3 ) 
return valid ; 

idx = ( void * ) ( idx_ie + 2 ) ; 

if ( dtim_count ) 
* dtim_count = idx -> dtim_count ; 

if ( dtim_period ) 
* dtim_period = idx -> dtim_period ; 

return true ; 
} 

static bool ieee80211_assoc_success ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_mgmt * mgmt , 
struct ieee802_11_elems * elems , 
const u8 * elem_start , unsigned int elem_len ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct ieee80211_mgd_assoc_data * assoc_data = ifmgd -> assoc_data ; 
struct ieee80211_local * local = sdata -> local ; 
unsigned int link_id ; 
struct sta_info * sta ; 
u64 changed [ ieee80211_mld_max_num_links ] = { } ; 
u16 valid_links = 0 ; 
int err ; 

mutex_lock ( & sdata -> local -> sta_mtx ) ; 




sta = sta_info_get ( sdata , assoc_data -> ap_addr ) ; 
if ( warn_on ( ! sta ) ) 
goto out_err ; 

if ( sdata -> vif . valid_links ) { 
for ( link_id = 0 ; link_id < ieee80211_mld_max_num_links ; link_id ++ ) { 
if ( ! assoc_data -> link [ link_id ] . bss ) 
continue ; 
valid_links |= bit ( link_id ) ; 

if ( link_id != assoc_data -> assoc_link_id ) { 
err = ieee80211_sta_allocate_link ( sta , link_id ) ; 
if ( err ) 
goto out_err ; 
} 
} 

ieee80211_vif_set_links ( sdata , valid_links ) ; 
} 

for ( link_id = 0 ; link_id < ieee80211_mld_max_num_links ; link_id ++ ) { 
struct cfg80211_bss * cbss = assoc_data -> link [ link_id ] . bss ; 
struct ieee80211_link_data * link ; 
struct link_sta_info * link_sta ; 

if ( ! cbss ) 
continue ; 

link = sdata_dereference ( sdata -> link [ link_id ] , sdata ) ; 
if ( warn_on ( ! link ) ) 
goto out_err ; 

if ( sdata -> vif . valid_links ) 
link_info ( link , 
"local address %pm, ap link address %pm%s\n" , 
link -> conf -> addr , 
assoc_data -> link [ link_id ] . bss -> bssid , 
link_id == assoc_data -> assoc_link_id ? 
" (assoc)" : "" ) ; 

link_sta = rcu_dereference_protected ( sta -> link [ link_id ] , 
lockdep_is_held ( & local -> sta_mtx ) ) ; 
if ( warn_on ( ! link_sta ) ) 
goto out_err ; 

if ( ! link -> u . mgd . have_beacon ) { 
const struct cfg80211_bss_ies * ies ; 

rcu_read_lock ( ) ; 
ies = rcu_dereference ( cbss -> beacon_ies ) ; 
if ( ies ) 
link -> u . mgd . have_beacon = true ; 
else 
ies = rcu_dereference ( cbss -> ies ) ; 
ieee80211_get_dtim ( ies , 
& link -> conf -> sync_dtim_count , 
& link -> u . mgd . dtim_period ) ; 
link -> conf -> beacon_int = cbss -> beacon_interval ; 
rcu_read_unlock ( ) ; 
} 

link -> conf -> dtim_period = link -> u . mgd . dtim_period ? : 1 ; 

if ( link_id != assoc_data -> assoc_link_id ) { 
err = ieee80211_prep_channel ( sdata , link , cbss , 
& link -> u . mgd . conn_flags ) ; 
if ( err ) { 
link_info ( link , "prep_channel failed\n" ) ; 
goto out_err ; 
} 
} 

err = ieee80211_mgd_setup_link_sta ( link , sta , link_sta , 
assoc_data -> link [ link_id ] . bss ) ; 
if ( err ) 
goto out_err ; 

if ( ! ieee80211_assoc_config_link ( link , link_sta , 
assoc_data -> link [ link_id ] . bss , 
mgmt , elem_start , elem_len , 
& changed [ link_id ] ) ) 
goto out_err ; 

if ( assoc_data -> link [ link_id ] . status != wlan_status_success ) { 
valid_links &= ~ bit ( link_id ) ; 
ieee80211_sta_remove_link ( sta , link_id ) ; 
continue ; 
} 

if ( link_id != assoc_data -> assoc_link_id ) { 
err = ieee80211_sta_activate_link ( sta , link_id ) ; 
if ( err ) 
goto out_err ; 
} 
} 


ieee80211_vif_set_links ( sdata , valid_links ) ; 

rate_control_rate_init ( sta ) ; 

if ( ifmgd -> flags & ieee80211_sta_mfp_enabled ) { 
set_sta_flag ( sta , wlan_sta_mfp ) ; 
sta -> sta . mfp = true ; 
} else { 
sta -> sta . mfp = false ; 
} 

ieee80211_sta_set_max_amsdu_subframes ( sta , elems -> ext_capab , 
elems -> ext_capab_len ) ; 

sta -> sta . wme = ( elems -> wmm_param || elems -> s1g_capab ) && 
local -> hw . queues >= ieee80211_num_acs ; 

err = sta_info_move_state ( sta , ieee80211_sta_assoc ) ; 
if ( ! err && ! ( ifmgd -> flags & ieee80211_sta_control_port ) ) 
err = sta_info_move_state ( sta , ieee80211_sta_authorized ) ; 
if ( err ) { 
sdata_info ( sdata , 
"failed to move station %pm to desired state\n" , 
sta -> sta . addr ) ; 
warn_on ( __sta_info_destroy ( sta ) ) ; 
goto out_err ; 
} 

if ( sdata -> wdev . use_4addr ) 
drv_sta_set_4addr ( local , sdata , & sta -> sta , true ) ; 

mutex_unlock ( & sdata -> local -> sta_mtx ) ; 

ieee80211_set_associated ( sdata , assoc_data , changed ) ; 





if ( ifmgd -> use_4addr ) 
ieee80211_send_4addr_nullfunc ( local , sdata ) ; 





ieee80211_sta_reset_beacon_monitor ( sdata ) ; 
ieee80211_sta_reset_conn_monitor ( sdata ) ; 

return true ; 
out_err : 
eth_zero_addr ( sdata -> vif . cfg . ap_addr ) ; 
mutex_unlock ( & sdata -> local -> sta_mtx ) ; 
return false ; 
} 

static void ieee80211_rx_mgmt_assoc_resp ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_mgmt * mgmt , 
size_t len ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct ieee80211_mgd_assoc_data * assoc_data = ifmgd -> assoc_data ; 
u16 capab_info , status_code , aid ; 
struct ieee80211_elems_parse_params parse_params = { 
. bss = null , 
. link_id = - 1 , 
. from_ap = true , 
} ; 
struct ieee802_11_elems * elems ; 
int ac ; 
const u8 * elem_start ; 
unsigned int elem_len ; 
bool reassoc ; 
struct ieee80211_event event = { 
. type = mlme_event , 
. u . mlme . data = assoc_event , 
} ; 
struct ieee80211_prep_tx_info info = { } ; 
struct cfg80211_rx_assoc_resp resp = { 
. uapsd_queues = - 1 , 
} ; 
u8 ap_mld_addr [ eth_alen ] __aligned ( 2 ) ; 
unsigned int link_id ; 

sdata_assert_lock ( sdata ) ; 

if ( ! assoc_data ) 
return ; 

if ( ! ether_addr_equal ( assoc_data -> ap_addr , mgmt -> bssid ) || 
! ether_addr_equal ( assoc_data -> ap_addr , mgmt -> sa ) ) 
return ; 






if ( len < 24 + 6 ) 
return ; 

reassoc = ieee80211_is_reassoc_resp ( mgmt -> frame_control ) ; 
capab_info = le16_to_cpu ( mgmt -> u . assoc_resp . capab_info ) ; 
status_code = le16_to_cpu ( mgmt -> u . assoc_resp . status_code ) ; 
if ( assoc_data -> s1g ) 
elem_start = mgmt -> u . s1g_assoc_resp . variable ; 
else 
elem_start = mgmt -> u . assoc_resp . variable ; 







info . subtype = reassoc ? ieee80211_stype_reassoc_req : 
ieee80211_stype_assoc_req ; 

if ( assoc_data -> fils_kek_len && 
fils_decrypt_assoc_resp ( sdata , ( u8 * ) mgmt , & len , assoc_data ) < 0 ) 
return ; 

elem_len = len - ( elem_start - ( u8 * ) mgmt ) ; 
parse_params . start = elem_start ; 
parse_params . len = elem_len ; 
elems = ieee802_11_parse_elems_full ( & parse_params ) ; 
if ( ! elems ) 
goto notify_driver ; 

if ( elems -> aid_resp ) 
aid = le16_to_cpu ( elems -> aid_resp -> aid ) ; 
else if ( assoc_data -> s1g ) 
aid = 0 ; 
else 
aid = le16_to_cpu ( mgmt -> u . assoc_resp . aid ) ; 





aid &= 0x7ff ; 

sdata_info ( sdata , 
"rx %sssocresp from %pm (capab=0x%x status=%d aid=%d)\n" , 
reassoc ? "rea" : "a" , assoc_data -> ap_addr , 
capab_info , status_code , ( u16 ) ( aid & ~ ( bit ( 15 ) | bit ( 14 ) ) ) ) ; 

ifmgd -> broken_ap = false ; 

if ( status_code == wlan_status_assoc_rejected_temporarily && 
elems -> timeout_int && 
elems -> timeout_int -> type == wlan_timeout_assoc_comeback ) { 
u32 tu , ms ; 

cfg80211_assoc_comeback ( sdata -> dev , assoc_data -> ap_addr , 
le32_to_cpu ( elems -> timeout_int -> value ) ) ; 

tu = le32_to_cpu ( elems -> timeout_int -> value ) ; 
ms = tu * 1024 / 1000 ; 
sdata_info ( sdata , 
"%pm rejected association temporarily; comeback duration %u tu (%u ms)\n" , 
assoc_data -> ap_addr , tu , ms ) ; 
assoc_data -> timeout = jiffies + msecs_to_jiffies ( ms ) ; 
assoc_data -> timeout_started = true ; 
if ( ms > ieee80211_assoc_timeout ) 
run_again ( sdata , assoc_data -> timeout ) ; 
goto notify_driver ; 
} 

if ( status_code != wlan_status_success ) { 
sdata_info ( sdata , "%pm denied association (code=%d)\n" , 
assoc_data -> ap_addr , status_code ) ; 
event . u . mlme . status = mlme_denied ; 
event . u . mlme . reason = status_code ; 
drv_event_callback ( sdata -> local , sdata , & event ) ; 
} else { 
if ( aid == 0 || aid > ieee80211_max_aid ) { 
sdata_info ( sdata , 
"invalid aid value %d (out of range), turn off ps\n" , 
aid ) ; 
aid = 0 ; 
ifmgd -> broken_ap = true ; 
} 

if ( sdata -> vif . valid_links ) { 
if ( ! elems -> multi_link ) { 
sdata_info ( sdata , 
"mlo association with %pm but no multi-link element in response!\n" , 
assoc_data -> ap_addr ) ; 
goto abandon_assoc ; 
} 

if ( le16_get_bits ( elems -> multi_link -> control , 
ieee80211_ml_control_type ) != 
ieee80211_ml_control_type_basic ) { 
sdata_info ( sdata , 
"bad multi-link element (control=0x%x)\n" , 
le16_to_cpu ( elems -> multi_link -> control ) ) ; 
goto abandon_assoc ; 
} else { 
struct ieee80211_mle_basic_common_info * common ; 

common = ( void * ) elems -> multi_link -> variable ; 

if ( memcmp ( assoc_data -> ap_addr , 
common -> mld_mac_addr , eth_alen ) ) { 
sdata_info ( sdata , 
"ap mld mac address mismatch: got %pm expected %pm\n" , 
common -> mld_mac_addr , 
assoc_data -> ap_addr ) ; 
goto abandon_assoc ; 
} 
} 
} 

sdata -> vif . cfg . aid = aid ; 

if ( ! ieee80211_assoc_success ( sdata , mgmt , elems , 
elem_start , elem_len ) ) { 

ieee80211_destroy_assoc_data ( sdata , assoc_timeout ) ; 
goto notify_driver ; 
} 
event . u . mlme . status = mlme_success ; 
drv_event_callback ( sdata -> local , sdata , & event ) ; 
sdata_info ( sdata , "associated\n" ) ; 

info . success = 1 ; 
} 

for ( link_id = 0 ; link_id < ieee80211_mld_max_num_links ; link_id ++ ) { 
struct ieee80211_link_data * link ; 

link = sdata_dereference ( sdata -> link [ link_id ] , sdata ) ; 
if ( ! link ) 
continue ; 

if ( ! assoc_data -> link [ link_id ] . bss ) 
continue ; 

resp . links [ link_id ] . bss = assoc_data -> link [ link_id ] . bss ; 
resp . links [ link_id ] . addr = link -> conf -> addr ; 
resp . links [ link_id ] . status = assoc_data -> link [ link_id ] . status ; 


resp . uapsd_queues = 0 ; 
for ( ac = 0 ; ac < ieee80211_num_acs ; ac ++ ) 
if ( link -> tx_conf [ ac ] . uapsd ) 
resp . uapsd_queues |= ieee80211_ac_to_qos_mask [ ac ] ; 
} 

if ( sdata -> vif . valid_links ) { 
ether_addr_copy ( ap_mld_addr , sdata -> vif . cfg . ap_addr ) ; 
resp . ap_mld_addr = ap_mld_addr ; 
} 

ieee80211_destroy_assoc_data ( sdata , 
status_code == wlan_status_success ? 
assoc_success : 
assoc_rejected ) ; 

resp . buf = ( u8 * ) mgmt ; 
resp . len = len ; 
resp . req_ies = ifmgd -> assoc_req_ies ; 
resp . req_ies_len = ifmgd -> assoc_req_ies_len ; 
cfg80211_rx_assoc_resp ( sdata -> dev , & resp ) ; 
notify_driver : 
drv_mgd_complete_tx ( sdata -> local , sdata , & info ) ; 
kfree ( elems ) ; 
return ; 
abandon_assoc : 
ieee80211_destroy_assoc_data ( sdata , assoc_abandon ) ; 
goto notify_driver ; 
} 

static void ieee80211_rx_bss_info ( struct ieee80211_link_data * link , 
struct ieee80211_mgmt * mgmt , size_t len , 
struct ieee80211_rx_status * rx_status ) 
{ 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_bss * bss ; 
struct ieee80211_channel * channel ; 

sdata_assert_lock ( sdata ) ; 

channel = ieee80211_get_channel_khz ( local -> hw . wiphy , 
ieee80211_rx_status_to_khz ( rx_status ) ) ; 
if ( ! channel ) 
return ; 

bss = ieee80211_bss_info_update ( local , rx_status , mgmt , len , channel ) ; 
if ( bss ) { 
link -> conf -> beacon_rate = bss -> beacon_rate ; 
ieee80211_rx_bss_put ( local , bss ) ; 
} 
} 


static void ieee80211_rx_mgmt_probe_resp ( struct ieee80211_link_data * link , 
struct sk_buff * skb ) 
{ 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 
struct ieee80211_mgmt * mgmt = ( void * ) skb -> data ; 
struct ieee80211_if_managed * ifmgd ; 
struct ieee80211_rx_status * rx_status = ( void * ) skb -> cb ; 
struct ieee80211_channel * channel ; 
size_t baselen , len = skb -> len ; 

ifmgd = & sdata -> u . mgd ; 

sdata_assert_lock ( sdata ) ; 








channel = ieee80211_get_channel ( sdata -> local -> hw . wiphy , 
rx_status -> freq ) ; 
if ( ! channel ) 
return ; 

if ( ! ether_addr_equal ( mgmt -> da , sdata -> vif . addr ) && 
( channel -> band != nl80211_band_6ghz || 
! is_broadcast_ether_addr ( mgmt -> da ) ) ) 
return ; 

baselen = ( u8 * ) mgmt -> u . probe_resp . variable - ( u8 * ) mgmt ; 
if ( baselen > len ) 
return ; 

ieee80211_rx_bss_info ( link , mgmt , len , rx_status ) ; 

if ( ifmgd -> associated && 
ether_addr_equal ( mgmt -> bssid , link -> u . mgd . bssid ) ) 
ieee80211_reset_ap_probe ( sdata ) ; 
} 
















static const u64 care_about_ies = 
( 1ull << wlan_eid_country ) | 
( 1ull << wlan_eid_erp_info ) | 
( 1ull << wlan_eid_channel_switch ) | 
( 1ull << wlan_eid_pwr_constraint ) | 
( 1ull << wlan_eid_ht_capability ) | 
( 1ull << wlan_eid_ht_operation ) | 
( 1ull << wlan_eid_ext_chanswitch_ann ) ; 

static void ieee80211_handle_beacon_sig ( struct ieee80211_link_data * link , 
struct ieee80211_if_managed * ifmgd , 
struct ieee80211_bss_conf * bss_conf , 
struct ieee80211_local * local , 
struct ieee80211_rx_status * rx_status ) 
{ 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 



if ( ! link -> u . mgd . tracking_signal_avg ) { 
link -> u . mgd . tracking_signal_avg = true ; 
ewma_beacon_signal_init ( & link -> u . mgd . ave_beacon_signal ) ; 
link -> u . mgd . last_cqm_event_signal = 0 ; 
link -> u . mgd . count_beacon_signal = 1 ; 
link -> u . mgd . last_ave_beacon_signal = 0 ; 
} else { 
link -> u . mgd . count_beacon_signal ++ ; 
} 

ewma_beacon_signal_add ( & link -> u . mgd . ave_beacon_signal , 
- rx_status -> signal ) ; 

if ( ifmgd -> rssi_min_thold != ifmgd -> rssi_max_thold && 
link -> u . mgd . count_beacon_signal >= ieee80211_signal_ave_min_count ) { 
int sig = - ewma_beacon_signal_read ( & link -> u . mgd . ave_beacon_signal ) ; 
int last_sig = link -> u . mgd . last_ave_beacon_signal ; 
struct ieee80211_event event = { 
. type = rssi_event , 
} ; 





if ( sig > ifmgd -> rssi_max_thold && 
( last_sig <= ifmgd -> rssi_min_thold || last_sig == 0 ) ) { 
link -> u . mgd . last_ave_beacon_signal = sig ; 
event . u . rssi . data = rssi_event_high ; 
drv_event_callback ( local , sdata , & event ) ; 
} else if ( sig < ifmgd -> rssi_min_thold && 
( last_sig >= ifmgd -> rssi_max_thold || 
last_sig == 0 ) ) { 
link -> u . mgd . last_ave_beacon_signal = sig ; 
event . u . rssi . data = rssi_event_low ; 
drv_event_callback ( local , sdata , & event ) ; 
} 
} 

if ( bss_conf -> cqm_rssi_thold && 
link -> u . mgd . count_beacon_signal >= ieee80211_signal_ave_min_count && 
! ( sdata -> vif . driver_flags & ieee80211_vif_supports_cqm_rssi ) ) { 
int sig = - ewma_beacon_signal_read ( & link -> u . mgd . ave_beacon_signal ) ; 
int last_event = link -> u . mgd . last_cqm_event_signal ; 
int thold = bss_conf -> cqm_rssi_thold ; 
int hyst = bss_conf -> cqm_rssi_hyst ; 

if ( sig < thold && 
( last_event == 0 || sig < last_event - hyst ) ) { 
link -> u . mgd . last_cqm_event_signal = sig ; 
ieee80211_cqm_rssi_notify ( 
& sdata -> vif , 
nl80211_cqm_rssi_threshold_event_low , 
sig , gfp_kernel ) ; 
} else if ( sig > thold && 
( last_event == 0 || sig > last_event + hyst ) ) { 
link -> u . mgd . last_cqm_event_signal = sig ; 
ieee80211_cqm_rssi_notify ( 
& sdata -> vif , 
nl80211_cqm_rssi_threshold_event_high , 
sig , gfp_kernel ) ; 
} 
} 

if ( bss_conf -> cqm_rssi_low && 
link -> u . mgd . count_beacon_signal >= ieee80211_signal_ave_min_count ) { 
int sig = - ewma_beacon_signal_read ( & link -> u . mgd . ave_beacon_signal ) ; 
int last_event = link -> u . mgd . last_cqm_event_signal ; 
int low = bss_conf -> cqm_rssi_low ; 
int high = bss_conf -> cqm_rssi_high ; 

if ( sig < low && 
( last_event == 0 || last_event >= low ) ) { 
link -> u . mgd . last_cqm_event_signal = sig ; 
ieee80211_cqm_rssi_notify ( 
& sdata -> vif , 
nl80211_cqm_rssi_threshold_event_low , 
sig , gfp_kernel ) ; 
} else if ( sig > high && 
( last_event == 0 || last_event <= high ) ) { 
link -> u . mgd . last_cqm_event_signal = sig ; 
ieee80211_cqm_rssi_notify ( 
& sdata -> vif , 
nl80211_cqm_rssi_threshold_event_high , 
sig , gfp_kernel ) ; 
} 
} 
} 

static bool ieee80211_rx_our_beacon ( const u8 * tx_bssid , 
struct cfg80211_bss * bss ) 
{ 
if ( ether_addr_equal ( tx_bssid , bss -> bssid ) ) 
return true ; 
if ( ! bss -> transmitted_bss ) 
return false ; 
return ether_addr_equal ( tx_bssid , bss -> transmitted_bss -> bssid ) ; 
} 

static bool ieee80211_config_puncturing ( struct ieee80211_link_data * link , 
const struct ieee80211_eht_operation * eht_oper , 
u64 * changed ) 
{ 
u16 bitmap = 0 , extracted ; 

if ( ( eht_oper -> params & ieee80211_eht_oper_info_present ) && 
( eht_oper -> params & 
ieee80211_eht_oper_disabled_subchannel_bitmap_present ) ) { 
const struct ieee80211_eht_operation_info * info = 
( void * ) eht_oper -> optional ; 
const u8 * disable_subchannel_bitmap = info -> optional ; 

bitmap = get_unaligned_le16 ( disable_subchannel_bitmap ) ; 
} 

extracted = ieee80211_extract_dis_subch_bmap ( eht_oper , 
& link -> conf -> chandef , 
bitmap ) ; 


if ( ! ( * changed & bss_changed_bandwidth ) && 
extracted == link -> conf -> eht_puncturing ) 
return true ; 

if ( ! cfg80211_valid_disable_subchannel_bitmap ( & bitmap , 
& link -> conf -> chandef ) ) { 
link_info ( link , 
"got an invalid disable subchannel bitmap from ap %pm: bitmap = 0x%x, bw = 0x%x. disconnect\n" , 
link -> u . mgd . bssid , 
bitmap , 
link -> conf -> chandef . width ) ; 
return false ; 
} 

ieee80211_handle_puncturing_bitmap ( link , eht_oper , bitmap , changed ) ; 
return true ; 
} 

static void ieee80211_rx_mgmt_beacon ( struct ieee80211_link_data * link , 
struct ieee80211_hdr * hdr , size_t len , 
struct ieee80211_rx_status * rx_status ) 
{ 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct ieee80211_bss_conf * bss_conf = & sdata -> vif . bss_conf ; 
struct ieee80211_vif_cfg * vif_cfg = & sdata -> vif . cfg ; 
struct ieee80211_mgmt * mgmt = ( void * ) hdr ; 
size_t baselen ; 
struct ieee802_11_elems * elems ; 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_chanctx_conf * chanctx_conf ; 
struct ieee80211_supported_band * sband ; 
struct ieee80211_channel * chan ; 
struct link_sta_info * link_sta ; 
struct sta_info * sta ; 
u64 changed = 0 ; 
bool erp_valid ; 
u8 erp_value = 0 ; 
u32 ncrc = 0 ; 
u8 * bssid , * variable = mgmt -> u . beacon . variable ; 
u8 deauth_buf [ ieee80211_deauth_frame_len ] ; 
struct ieee80211_elems_parse_params parse_params = { 
. link_id = - 1 , 
. from_ap = true , 
} ; 

sdata_assert_lock ( sdata ) ; 


bssid = ieee80211_get_bssid ( hdr , len , sdata -> vif . type ) ; 
if ( ieee80211_is_s1g_beacon ( mgmt -> frame_control ) ) { 
struct ieee80211_ext * ext = ( void * ) mgmt ; 

if ( ieee80211_is_s1g_short_beacon ( ext -> frame_control ) ) 
variable = ext -> u . s1g_short_beacon . variable ; 
else 
variable = ext -> u . s1g_beacon . variable ; 
} 

baselen = ( u8 * ) variable - ( u8 * ) mgmt ; 
if ( baselen > len ) 
return ; 

parse_params . start = variable ; 
parse_params . len = len - baselen ; 

rcu_read_lock ( ) ; 
chanctx_conf = rcu_dereference ( link -> conf -> chanctx_conf ) ; 
if ( ! chanctx_conf ) { 
rcu_read_unlock ( ) ; 
return ; 
} 

if ( ieee80211_rx_status_to_khz ( rx_status ) != 
ieee80211_channel_to_khz ( chanctx_conf -> def . chan ) ) { 
rcu_read_unlock ( ) ; 
return ; 
} 
chan = chanctx_conf -> def . chan ; 
rcu_read_unlock ( ) ; 

if ( ifmgd -> assoc_data && ifmgd -> assoc_data -> need_beacon && 
! warn_on ( sdata -> vif . valid_links ) && 
ieee80211_rx_our_beacon ( bssid , ifmgd -> assoc_data -> link [ 0 ] . bss ) ) { 
parse_params . bss = ifmgd -> assoc_data -> link [ 0 ] . bss ; 
elems = ieee802_11_parse_elems_full ( & parse_params ) ; 
if ( ! elems ) 
return ; 

ieee80211_rx_bss_info ( link , mgmt , len , rx_status ) ; 

if ( elems -> dtim_period ) 
link -> u . mgd . dtim_period = elems -> dtim_period ; 
link -> u . mgd . have_beacon = true ; 
ifmgd -> assoc_data -> need_beacon = false ; 
if ( ieee80211_hw_check ( & local -> hw , timing_beacon_only ) ) { 
link -> conf -> sync_tsf = 
le64_to_cpu ( mgmt -> u . beacon . timestamp ) ; 
link -> conf -> sync_device_ts = 
rx_status -> device_timestamp ; 
link -> conf -> sync_dtim_count = elems -> dtim_count ; 
} 

if ( elems -> mbssid_config_ie ) 
bss_conf -> profile_periodicity = 
elems -> mbssid_config_ie -> profile_periodicity ; 
else 
bss_conf -> profile_periodicity = 0 ; 

if ( elems -> ext_capab_len >= 11 && 
( elems -> ext_capab [ 10 ] & wlan_ext_capa11_ema_support ) ) 
bss_conf -> ema_ap = true ; 
else 
bss_conf -> ema_ap = false ; 


ifmgd -> assoc_data -> timeout = jiffies ; 
ifmgd -> assoc_data -> timeout_started = true ; 
run_again ( sdata , ifmgd -> assoc_data -> timeout ) ; 
kfree ( elems ) ; 
return ; 
} 

if ( ! ifmgd -> associated || 
! ieee80211_rx_our_beacon ( bssid , link -> u . mgd . bss ) ) 
return ; 
bssid = link -> u . mgd . bssid ; 

if ( ! ( rx_status -> flag & rx_flag_no_signal_val ) ) 
ieee80211_handle_beacon_sig ( link , ifmgd , bss_conf , 
local , rx_status ) ; 

if ( ifmgd -> flags & ieee80211_sta_connection_poll ) { 
mlme_dbg_ratelimited ( sdata , 
"cancelling ap probe due to a received beacon\n" ) ; 
ieee80211_reset_ap_probe ( sdata ) ; 
} 





ieee80211_sta_reset_beacon_monitor ( sdata ) ; 






if ( ! ieee80211_is_s1g_beacon ( hdr -> frame_control ) ) 
ncrc = crc32_be ( 0 , ( void * ) & mgmt -> u . beacon . beacon_int , 4 ) ; 
parse_params . bss = link -> u . mgd . bss ; 
parse_params . filter = care_about_ies ; 
parse_params . crc = ncrc ; 
elems = ieee802_11_parse_elems_full ( & parse_params ) ; 
if ( ! elems ) 
return ; 
ncrc = elems -> crc ; 

if ( ieee80211_hw_check ( & local -> hw , ps_nullfunc_stack ) && 
ieee80211_check_tim ( elems -> tim , elems -> tim_len , vif_cfg -> aid ) ) { 
if ( local -> hw . conf . dynamic_ps_timeout > 0 ) { 
if ( local -> hw . conf . flags & ieee80211_conf_ps ) { 
local -> hw . conf . flags &= ~ ieee80211_conf_ps ; 
ieee80211_hw_config ( local , 
ieee80211_conf_change_ps ) ; 
} 
ieee80211_send_nullfunc ( local , sdata , false ) ; 
} else if ( ! local -> pspolling && sdata -> u . mgd . powersave ) { 
local -> pspolling = true ; 









ieee80211_send_pspoll ( local , sdata ) ; 
} 
} 

if ( sdata -> vif . p2p || 
sdata -> vif . driver_flags & ieee80211_vif_get_noa_update ) { 
struct ieee80211_p2p_noa_attr noa = { } ; 
int ret ; 

ret = cfg80211_get_p2p_attr ( variable , 
len - baselen , 
ieee80211_p2p_attr_absence_notice , 
( u8 * ) & noa , sizeof ( noa ) ) ; 
if ( ret >= 2 ) { 
if ( link -> u . mgd . p2p_noa_index != noa . index ) { 

link -> u . mgd . p2p_noa_index = noa . index ; 
memcpy ( & bss_conf -> p2p_noa_attr , & noa , sizeof ( noa ) ) ; 
changed |= bss_changed_p2p_ps ; 




link -> u . mgd . beacon_crc_valid = false ; 
} 
} else if ( link -> u . mgd . p2p_noa_index != - 1 ) { 

link -> u . mgd . p2p_noa_index = - 1 ; 
memset ( & bss_conf -> p2p_noa_attr , 0 , sizeof ( bss_conf -> p2p_noa_attr ) ) ; 
changed |= bss_changed_p2p_ps ; 
link -> u . mgd . beacon_crc_valid = false ; 
} 
} 

if ( link -> u . mgd . csa_waiting_bcn ) 
ieee80211_chswitch_post_beacon ( link ) ; 









if ( ieee80211_hw_check ( & local -> hw , timing_beacon_only ) && 
! ieee80211_is_s1g_beacon ( hdr -> frame_control ) ) { 
link -> conf -> sync_tsf = 
le64_to_cpu ( mgmt -> u . beacon . timestamp ) ; 
link -> conf -> sync_device_ts = 
rx_status -> device_timestamp ; 
link -> conf -> sync_dtim_count = elems -> dtim_count ; 
} 

if ( ( ncrc == link -> u . mgd . beacon_crc && link -> u . mgd . beacon_crc_valid ) || 
ieee80211_is_s1g_short_beacon ( mgmt -> frame_control ) ) 
goto free ; 
link -> u . mgd . beacon_crc = ncrc ; 
link -> u . mgd . beacon_crc_valid = true ; 

ieee80211_rx_bss_info ( link , mgmt , len , rx_status ) ; 

ieee80211_sta_process_chanswitch ( link , rx_status -> mactime , 
rx_status -> device_timestamp , 
elems , true ) ; 

if ( ! link -> u . mgd . disable_wmm_tracking && 
ieee80211_sta_wmm_params ( local , link , elems -> wmm_param , 
elems -> wmm_param_len , 
elems -> mu_edca_param_set ) ) 
changed |= bss_changed_qos ; 





if ( ! link -> u . mgd . have_beacon ) { 

bss_conf -> dtim_period = elems -> dtim_period ? : 1 ; 

changed |= bss_changed_beacon_info ; 
link -> u . mgd . have_beacon = true ; 

mutex_lock ( & local -> iflist_mtx ) ; 
ieee80211_recalc_ps ( local ) ; 
mutex_unlock ( & local -> iflist_mtx ) ; 

ieee80211_recalc_ps_vif ( sdata ) ; 
} 

if ( elems -> erp_info ) { 
erp_valid = true ; 
erp_value = elems -> erp_info [ 0 ] ; 
} else { 
erp_valid = false ; 
} 

if ( ! ieee80211_is_s1g_beacon ( hdr -> frame_control ) ) 
changed |= ieee80211_handle_bss_capability ( link , 
le16_to_cpu ( mgmt -> u . beacon . capab_info ) , 
erp_valid , erp_value ) ; 

mutex_lock ( & local -> sta_mtx ) ; 
sta = sta_info_get ( sdata , sdata -> vif . cfg . ap_addr ) ; 
if ( warn_on ( ! sta ) ) { 
mutex_unlock ( & local -> sta_mtx ) ; 
goto free ; 
} 
link_sta = rcu_dereference_protected ( sta -> link [ link -> link_id ] , 
lockdep_is_held ( & local -> sta_mtx ) ) ; 
if ( warn_on ( ! link_sta ) ) { 
mutex_unlock ( & local -> sta_mtx ) ; 
goto free ; 
} 

if ( warn_on ( ! link -> conf -> chandef . chan ) ) 
goto free ; 

sband = local -> hw . wiphy -> bands [ link -> conf -> chandef . chan -> band ] ; 

changed |= ieee80211_recalc_twt_req ( sdata , sband , link , link_sta , elems ) ; 

if ( ieee80211_config_bw ( link , elems -> ht_cap_elem , 
elems -> vht_cap_elem , elems -> ht_operation , 
elems -> vht_operation , elems -> he_operation , 
elems -> eht_operation , 
elems -> s1g_oper , bssid , & changed ) ) { 
mutex_unlock ( & local -> sta_mtx ) ; 
sdata_info ( sdata , 
"failed to follow ap %pm bandwidth change, disconnect\n" , 
bssid ) ; 
ieee80211_set_disassoc ( sdata , ieee80211_stype_deauth , 
wlan_reason_deauth_leaving , 
true , deauth_buf ) ; 
ieee80211_report_disconnect ( sdata , deauth_buf , 
sizeof ( deauth_buf ) , true , 
wlan_reason_deauth_leaving , 
false ) ; 
goto free ; 
} 

if ( sta && elems -> opmode_notif ) 
ieee80211_vht_handle_opmode ( sdata , link_sta , 
* elems -> opmode_notif , 
rx_status -> band ) ; 
mutex_unlock ( & local -> sta_mtx ) ; 

changed |= ieee80211_handle_pwr_constr ( link , chan , mgmt , 
elems -> country_elem , 
elems -> country_elem_len , 
elems -> pwr_constr_elem , 
elems -> cisco_dtpc_elem ) ; 

if ( elems -> eht_operation && 
! ( link -> u . mgd . conn_flags & ieee80211_conn_disable_eht ) ) { 
if ( ! ieee80211_config_puncturing ( link , elems -> eht_operation , 
& changed ) ) { 
ieee80211_set_disassoc ( sdata , ieee80211_stype_deauth , 
wlan_reason_deauth_leaving , 
true , deauth_buf ) ; 
ieee80211_report_disconnect ( sdata , deauth_buf , 
sizeof ( deauth_buf ) , true , 
wlan_reason_deauth_leaving , 
false ) ; 
goto free ; 
} 
} 

ieee80211_link_info_change_notify ( sdata , link , changed ) ; 
free : 
kfree ( elems ) ; 
} 

void ieee80211_sta_rx_queued_ext ( struct ieee80211_sub_if_data * sdata , 
struct sk_buff * skb ) 
{ 
struct ieee80211_link_data * link = & sdata -> deflink ; 
struct ieee80211_rx_status * rx_status ; 
struct ieee80211_hdr * hdr ; 
u16 fc ; 

rx_status = ( struct ieee80211_rx_status * ) skb -> cb ; 
hdr = ( struct ieee80211_hdr * ) skb -> data ; 
fc = le16_to_cpu ( hdr -> frame_control ) ; 

sdata_lock ( sdata ) ; 
switch ( fc & ieee80211_fctl_stype ) { 
case ieee80211_stype_s1g_beacon : 
ieee80211_rx_mgmt_beacon ( link , hdr , skb -> len , rx_status ) ; 
break ; 
} 
sdata_unlock ( sdata ) ; 
} 

void ieee80211_sta_rx_queued_mgmt ( struct ieee80211_sub_if_data * sdata , 
struct sk_buff * skb ) 
{ 
struct ieee80211_link_data * link = & sdata -> deflink ; 
struct ieee80211_rx_status * rx_status ; 
struct ieee80211_mgmt * mgmt ; 
u16 fc ; 
int ies_len ; 

rx_status = ( struct ieee80211_rx_status * ) skb -> cb ; 
mgmt = ( struct ieee80211_mgmt * ) skb -> data ; 
fc = le16_to_cpu ( mgmt -> frame_control ) ; 

sdata_lock ( sdata ) ; 

if ( rx_status -> link_valid ) { 
link = sdata_dereference ( sdata -> link [ rx_status -> link_id ] , 
sdata ) ; 
if ( ! link ) 
goto out ; 
} 

switch ( fc & ieee80211_fctl_stype ) { 
case ieee80211_stype_beacon : 
ieee80211_rx_mgmt_beacon ( link , ( void * ) mgmt , 
skb -> len , rx_status ) ; 
break ; 
case ieee80211_stype_probe_resp : 
ieee80211_rx_mgmt_probe_resp ( link , skb ) ; 
break ; 
case ieee80211_stype_auth : 
ieee80211_rx_mgmt_auth ( sdata , mgmt , skb -> len ) ; 
break ; 
case ieee80211_stype_deauth : 
ieee80211_rx_mgmt_deauth ( sdata , mgmt , skb -> len ) ; 
break ; 
case ieee80211_stype_disassoc : 
ieee80211_rx_mgmt_disassoc ( sdata , mgmt , skb -> len ) ; 
break ; 
case ieee80211_stype_assoc_resp : 
case ieee80211_stype_reassoc_resp : 
ieee80211_rx_mgmt_assoc_resp ( sdata , mgmt , skb -> len ) ; 
break ; 
case ieee80211_stype_action : 
if ( mgmt -> u . action . category == wlan_category_spectrum_mgmt ) { 
struct ieee802_11_elems * elems ; 

ies_len = skb -> len - 
offsetof ( struct ieee80211_mgmt , 
u . action . u . chan_switch . variable ) ; 

if ( ies_len < 0 ) 
break ; 


elems = ieee802_11_parse_elems ( 
mgmt -> u . action . u . chan_switch . variable , 
ies_len , true , null ) ; 

if ( elems && ! elems -> parse_error ) 
ieee80211_sta_process_chanswitch ( link , 
rx_status -> mactime , 
rx_status -> device_timestamp , 
elems , false ) ; 
kfree ( elems ) ; 
} else if ( mgmt -> u . action . category == wlan_category_public ) { 
struct ieee802_11_elems * elems ; 

ies_len = skb -> len - 
offsetof ( struct ieee80211_mgmt , 
u . action . u . ext_chan_switch . variable ) ; 

if ( ies_len < 0 ) 
break ; 





elems = ieee802_11_parse_elems ( 
mgmt -> u . action . u . ext_chan_switch . variable , 
ies_len , true , null ) ; 

if ( elems && ! elems -> parse_error ) { 

elems -> ext_chansw_ie = 
& mgmt -> u . action . u . ext_chan_switch . data ; 

ieee80211_sta_process_chanswitch ( link , 
rx_status -> mactime , 
rx_status -> device_timestamp , 
elems , false ) ; 
} 

kfree ( elems ) ; 
} 
break ; 
} 
out : 
sdata_unlock ( sdata ) ; 
} 

static void ieee80211_sta_timer ( struct timer_list * t ) 
{ 
struct ieee80211_sub_if_data * sdata = 
from_timer ( sdata , t , u . mgd . timer ) ; 

ieee80211_queue_work ( & sdata -> local -> hw , & sdata -> work ) ; 
} 

void ieee80211_sta_connection_lost ( struct ieee80211_sub_if_data * sdata , 
u8 reason , bool tx ) 
{ 
u8 frame_buf [ ieee80211_deauth_frame_len ] ; 

ieee80211_set_disassoc ( sdata , ieee80211_stype_deauth , reason , 
tx , frame_buf ) ; 

ieee80211_report_disconnect ( sdata , frame_buf , sizeof ( frame_buf ) , true , 
reason , false ) ; 
} 

static int ieee80211_auth ( struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct ieee80211_mgd_auth_data * auth_data = ifmgd -> auth_data ; 
u32 tx_flags = 0 ; 
u16 trans = 1 ; 
u16 status = 0 ; 
struct ieee80211_prep_tx_info info = { 
. subtype = ieee80211_stype_auth , 
} ; 

sdata_assert_lock ( sdata ) ; 

if ( warn_on_once ( ! auth_data ) ) 
return - einval ; 

auth_data -> tries ++ ; 

if ( auth_data -> tries > ieee80211_auth_max_tries ) { 
sdata_info ( sdata , "authentication with %pm timed out\n" , 
auth_data -> ap_addr ) ; 





cfg80211_unlink_bss ( local -> hw . wiphy , auth_data -> bss ) ; 

return - etimedout ; 
} 

if ( auth_data -> algorithm == wlan_auth_sae ) 
info . duration = jiffies_to_msecs ( ieee80211_auth_timeout_sae ) ; 

drv_mgd_prepare_tx ( local , sdata , & info ) ; 

sdata_info ( sdata , "send auth to %pm (try %d/%d)\n" , 
auth_data -> ap_addr , auth_data -> tries , 
ieee80211_auth_max_tries ) ; 

auth_data -> expected_transaction = 2 ; 

if ( auth_data -> algorithm == wlan_auth_sae ) { 
trans = auth_data -> sae_trans ; 
status = auth_data -> sae_status ; 
auth_data -> expected_transaction = trans ; 
} 

if ( ieee80211_hw_check ( & local -> hw , reports_tx_ack_status ) ) 
tx_flags = ieee80211_tx_ctl_req_tx_status | 
ieee80211_tx_intfl_mlme_conn_tx ; 

ieee80211_send_auth ( sdata , trans , auth_data -> algorithm , status , 
auth_data -> data , auth_data -> data_len , 
auth_data -> ap_addr , auth_data -> ap_addr , 
null , 0 , 0 , tx_flags ) ; 

if ( tx_flags == 0 ) { 
if ( auth_data -> algorithm == wlan_auth_sae ) 
auth_data -> timeout = jiffies + 
ieee80211_auth_timeout_sae ; 
else 
auth_data -> timeout = jiffies + ieee80211_auth_timeout ; 
} else { 
auth_data -> timeout = 
round_jiffies_up ( jiffies + ieee80211_auth_timeout_long ) ; 
} 

auth_data -> timeout_started = true ; 
run_again ( sdata , auth_data -> timeout ) ; 

return 0 ; 
} 

static int ieee80211_do_assoc ( struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_mgd_assoc_data * assoc_data = sdata -> u . mgd . assoc_data ; 
struct ieee80211_local * local = sdata -> local ; 
int ret ; 

sdata_assert_lock ( sdata ) ; 

assoc_data -> tries ++ ; 
if ( assoc_data -> tries > ieee80211_assoc_max_tries ) { 
sdata_info ( sdata , "association with %pm timed out\n" , 
assoc_data -> ap_addr ) ; 





cfg80211_unlink_bss ( local -> hw . wiphy , 
assoc_data -> link [ assoc_data -> assoc_link_id ] . bss ) ; 

return - etimedout ; 
} 

sdata_info ( sdata , "associate with %pm (try %d/%d)\n" , 
assoc_data -> ap_addr , assoc_data -> tries , 
ieee80211_assoc_max_tries ) ; 
ret = ieee80211_send_assoc ( sdata ) ; 
if ( ret ) 
return ret ; 

if ( ! ieee80211_hw_check ( & local -> hw , reports_tx_ack_status ) ) { 
assoc_data -> timeout = jiffies + ieee80211_assoc_timeout ; 
assoc_data -> timeout_started = true ; 
run_again ( sdata , assoc_data -> timeout ) ; 
} else { 
assoc_data -> timeout = 
round_jiffies_up ( jiffies + 
ieee80211_assoc_timeout_long ) ; 
assoc_data -> timeout_started = true ; 
run_again ( sdata , assoc_data -> timeout ) ; 
} 

return 0 ; 
} 

void ieee80211_mgd_conn_tx_status ( struct ieee80211_sub_if_data * sdata , 
__le16 fc , bool acked ) 
{ 
struct ieee80211_local * local = sdata -> local ; 

sdata -> u . mgd . status_fc = fc ; 
sdata -> u . mgd . status_acked = acked ; 
sdata -> u . mgd . status_received = true ; 

ieee80211_queue_work ( & local -> hw , & sdata -> work ) ; 
} 

void ieee80211_sta_work ( struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 

sdata_lock ( sdata ) ; 

if ( ifmgd -> status_received ) { 
__le16 fc = ifmgd -> status_fc ; 
bool status_acked = ifmgd -> status_acked ; 

ifmgd -> status_received = false ; 
if ( ifmgd -> auth_data && ieee80211_is_auth ( fc ) ) { 
if ( status_acked ) { 
if ( ifmgd -> auth_data -> algorithm == 
wlan_auth_sae ) 
ifmgd -> auth_data -> timeout = 
jiffies + 
ieee80211_auth_timeout_sae ; 
else 
ifmgd -> auth_data -> timeout = 
jiffies + 
ieee80211_auth_timeout_short ; 
run_again ( sdata , ifmgd -> auth_data -> timeout ) ; 
} else { 
ifmgd -> auth_data -> timeout = jiffies - 1 ; 
} 
ifmgd -> auth_data -> timeout_started = true ; 
} else if ( ifmgd -> assoc_data && 
( ieee80211_is_assoc_req ( fc ) || 
ieee80211_is_reassoc_req ( fc ) ) ) { 
if ( status_acked ) { 
ifmgd -> assoc_data -> timeout = 
jiffies + ieee80211_assoc_timeout_short ; 
run_again ( sdata , ifmgd -> assoc_data -> timeout ) ; 
} else { 
ifmgd -> assoc_data -> timeout = jiffies - 1 ; 
} 
ifmgd -> assoc_data -> timeout_started = true ; 
} 
} 

if ( ifmgd -> auth_data && ifmgd -> auth_data -> timeout_started && 
time_after ( jiffies , ifmgd -> auth_data -> timeout ) ) { 
if ( ifmgd -> auth_data -> done || ifmgd -> auth_data -> waiting ) { 




ieee80211_destroy_auth_data ( sdata , false ) ; 
} else if ( ieee80211_auth ( sdata ) ) { 
u8 ap_addr [ eth_alen ] ; 
struct ieee80211_event event = { 
. type = mlme_event , 
. u . mlme . data = auth_event , 
. u . mlme . status = mlme_timeout , 
} ; 

memcpy ( ap_addr , ifmgd -> auth_data -> ap_addr , eth_alen ) ; 

ieee80211_destroy_auth_data ( sdata , false ) ; 

cfg80211_auth_timeout ( sdata -> dev , ap_addr ) ; 
drv_event_callback ( sdata -> local , sdata , & event ) ; 
} 
} else if ( ifmgd -> auth_data && ifmgd -> auth_data -> timeout_started ) 
run_again ( sdata , ifmgd -> auth_data -> timeout ) ; 

if ( ifmgd -> assoc_data && ifmgd -> assoc_data -> timeout_started && 
time_after ( jiffies , ifmgd -> assoc_data -> timeout ) ) { 
if ( ( ifmgd -> assoc_data -> need_beacon && 
! sdata -> deflink . u . mgd . have_beacon ) || 
ieee80211_do_assoc ( sdata ) ) { 
struct ieee80211_event event = { 
. type = mlme_event , 
. u . mlme . data = assoc_event , 
. u . mlme . status = mlme_timeout , 
} ; 

ieee80211_destroy_assoc_data ( sdata , assoc_timeout ) ; 
drv_event_callback ( sdata -> local , sdata , & event ) ; 
} 
} else if ( ifmgd -> assoc_data && ifmgd -> assoc_data -> timeout_started ) 
run_again ( sdata , ifmgd -> assoc_data -> timeout ) ; 

if ( ifmgd -> flags & ieee80211_sta_connection_poll && 
ifmgd -> associated ) { 
u8 * bssid = sdata -> deflink . u . mgd . bssid ; 
int max_tries ; 

if ( ieee80211_hw_check ( & local -> hw , reports_tx_ack_status ) ) 
max_tries = max_nullfunc_tries ; 
else 
max_tries = max_probe_tries ; 


if ( ! ifmgd -> probe_send_count ) 
ieee80211_reset_ap_probe ( sdata ) ; 
else if ( ifmgd -> nullfunc_failed ) { 
if ( ifmgd -> probe_send_count < max_tries ) { 
mlme_dbg ( sdata , 
"no ack for nullfunc frame to ap %pm, try %d/%i\n" , 
bssid , ifmgd -> probe_send_count , 
max_tries ) ; 
ieee80211_mgd_probe_ap_send ( sdata ) ; 
} else { 
mlme_dbg ( sdata , 
"no ack for nullfunc frame to ap %pm, disconnecting.\n" , 
bssid ) ; 
ieee80211_sta_connection_lost ( sdata , 
wlan_reason_disassoc_due_to_inactivity , 
false ) ; 
} 
} else if ( time_is_after_jiffies ( ifmgd -> probe_timeout ) ) 
run_again ( sdata , ifmgd -> probe_timeout ) ; 
else if ( ieee80211_hw_check ( & local -> hw , reports_tx_ack_status ) ) { 
mlme_dbg ( sdata , 
"failed to send nullfunc to ap %pm after %dms, disconnecting\n" , 
bssid , probe_wait_ms ) ; 
ieee80211_sta_connection_lost ( sdata , 
wlan_reason_disassoc_due_to_inactivity , false ) ; 
} else if ( ifmgd -> probe_send_count < max_tries ) { 
mlme_dbg ( sdata , 
"no probe response from ap %pm after %dms, try %d/%i\n" , 
bssid , probe_wait_ms , 
ifmgd -> probe_send_count , max_tries ) ; 
ieee80211_mgd_probe_ap_send ( sdata ) ; 
} else { 




mlme_dbg ( sdata , 
"no probe response from ap %pm after %dms, disconnecting.\n" , 
bssid , probe_wait_ms ) ; 

ieee80211_sta_connection_lost ( sdata , 
wlan_reason_disassoc_due_to_inactivity , false ) ; 
} 
} 

sdata_unlock ( sdata ) ; 
} 

static void ieee80211_sta_bcn_mon_timer ( struct timer_list * t ) 
{ 
struct ieee80211_sub_if_data * sdata = 
from_timer ( sdata , t , u . mgd . bcn_mon_timer ) ; 

if ( warn_on ( sdata -> vif . valid_links ) ) 
return ; 

if ( sdata -> vif . bss_conf . csa_active && 
! sdata -> deflink . u . mgd . csa_waiting_bcn ) 
return ; 

if ( sdata -> vif . driver_flags & ieee80211_vif_beacon_filter ) 
return ; 

sdata -> u . mgd . connection_loss = false ; 
ieee80211_queue_work ( & sdata -> local -> hw , 
& sdata -> u . mgd . beacon_connection_loss_work ) ; 
} 

static void ieee80211_sta_conn_mon_timer ( struct timer_list * t ) 
{ 
struct ieee80211_sub_if_data * sdata = 
from_timer ( sdata , t , u . mgd . conn_mon_timer ) ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct ieee80211_local * local = sdata -> local ; 
struct sta_info * sta ; 
unsigned long timeout ; 

if ( warn_on ( sdata -> vif . valid_links ) ) 
return ; 

if ( sdata -> vif . bss_conf . csa_active && 
! sdata -> deflink . u . mgd . csa_waiting_bcn ) 
return ; 

sta = sta_info_get ( sdata , sdata -> vif . cfg . ap_addr ) ; 
if ( ! sta ) 
return ; 

timeout = sta -> deflink . status_stats . last_ack ; 
if ( time_before ( sta -> deflink . status_stats . last_ack , sta -> deflink . rx_stats . last_rx ) ) 
timeout = sta -> deflink . rx_stats . last_rx ; 
timeout += ieee80211_connection_idle_time ; 




if ( time_is_after_jiffies ( timeout ) ) { 
mod_timer ( & ifmgd -> conn_mon_timer , round_jiffies_up ( timeout ) ) ; 
return ; 
} 

ieee80211_queue_work ( & local -> hw , & ifmgd -> monitor_work ) ; 
} 

static void ieee80211_sta_monitor_work ( struct work_struct * work ) 
{ 
struct ieee80211_sub_if_data * sdata = 
container_of ( work , struct ieee80211_sub_if_data , 
u . mgd . monitor_work ) ; 

ieee80211_mgd_probe_ap ( sdata , false ) ; 
} 

static void ieee80211_restart_sta_timer ( struct ieee80211_sub_if_data * sdata ) 
{ 
if ( sdata -> vif . type == nl80211_iftype_station ) { 
__ieee80211_stop_poll ( sdata ) ; 


if ( ! ieee80211_hw_check ( & sdata -> local -> hw , connection_monitor ) ) 
ieee80211_queue_work ( & sdata -> local -> hw , 
& sdata -> u . mgd . monitor_work ) ; 
} 
} 


void ieee80211_mgd_quiesce ( struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
u8 frame_buf [ ieee80211_deauth_frame_len ] ; 

sdata_lock ( sdata ) ; 

if ( ifmgd -> auth_data || ifmgd -> assoc_data ) { 
const u8 * ap_addr = ifmgd -> auth_data ? 
ifmgd -> auth_data -> ap_addr : 
ifmgd -> assoc_data -> ap_addr ; 






ieee80211_send_deauth_disassoc ( sdata , ap_addr , ap_addr , 
ieee80211_stype_deauth , 
wlan_reason_deauth_leaving , 
false , frame_buf ) ; 
if ( ifmgd -> assoc_data ) 
ieee80211_destroy_assoc_data ( sdata , assoc_abandon ) ; 
if ( ifmgd -> auth_data ) 
ieee80211_destroy_auth_data ( sdata , false ) ; 
cfg80211_tx_mlme_mgmt ( sdata -> dev , frame_buf , 
ieee80211_deauth_frame_len , 
false ) ; 
} 


















if ( ifmgd -> associated && ! sdata -> local -> wowlan ) { 
u8 bssid [ eth_alen ] ; 
struct cfg80211_deauth_request req = { 
. reason_code = wlan_reason_deauth_leaving , 
. bssid = bssid , 
} ; 

memcpy ( bssid , sdata -> vif . cfg . ap_addr , eth_alen ) ; 
ieee80211_mgd_deauth ( sdata , & req ) ; 
} 

sdata_unlock ( sdata ) ; 
} 


void ieee80211_sta_restart ( struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 

sdata_lock ( sdata ) ; 
if ( ! ifmgd -> associated ) { 
sdata_unlock ( sdata ) ; 
return ; 
} 

if ( sdata -> flags & ieee80211_sdata_disconnect_resume ) { 
sdata -> flags &= ~ ieee80211_sdata_disconnect_resume ; 
mlme_dbg ( sdata , "driver requested disconnect after resume\n" ) ; 
ieee80211_sta_connection_lost ( sdata , 
wlan_reason_unspecified , 
true ) ; 
sdata_unlock ( sdata ) ; 
return ; 
} 

if ( sdata -> flags & ieee80211_sdata_disconnect_hw_restart ) { 
sdata -> flags &= ~ ieee80211_sdata_disconnect_hw_restart ; 
mlme_dbg ( sdata , "driver requested disconnect after hardware restart\n" ) ; 
ieee80211_sta_connection_lost ( sdata , 
wlan_reason_unspecified , 
true ) ; 
sdata_unlock ( sdata ) ; 
return ; 
} 

sdata_unlock ( sdata ) ; 
} 

static void ieee80211_request_smps_mgd_work ( struct work_struct * work ) 
{ 
struct ieee80211_link_data * link = 
container_of ( work , struct ieee80211_link_data , 
u . mgd . request_smps_work ) ; 

sdata_lock ( link -> sdata ) ; 
__ieee80211_request_smps_mgd ( link -> sdata , link , 
link -> u . mgd . driver_smps_mode ) ; 
sdata_unlock ( link -> sdata ) ; 
} 


void ieee80211_sta_setup_sdata ( struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 

init_work ( & ifmgd -> monitor_work , ieee80211_sta_monitor_work ) ; 
init_work ( & ifmgd -> beacon_connection_loss_work , 
ieee80211_beacon_connection_loss_work ) ; 
init_work ( & ifmgd -> csa_connection_drop_work , 
ieee80211_csa_connection_drop_work ) ; 
init_delayed_work ( & ifmgd -> tdls_peer_del_work , 
ieee80211_tdls_peer_del_work ) ; 
timer_setup ( & ifmgd -> timer , ieee80211_sta_timer , 0 ) ; 
timer_setup ( & ifmgd -> bcn_mon_timer , ieee80211_sta_bcn_mon_timer , 0 ) ; 
timer_setup ( & ifmgd -> conn_mon_timer , ieee80211_sta_conn_mon_timer , 0 ) ; 
init_delayed_work ( & ifmgd -> tx_tspec_wk , 
ieee80211_sta_handle_tspec_ac_params_wk ) ; 

ifmgd -> flags = 0 ; 
ifmgd -> powersave = sdata -> wdev . ps ; 
ifmgd -> uapsd_queues = sdata -> local -> hw . uapsd_queues ; 
ifmgd -> uapsd_max_sp_len = sdata -> local -> hw . uapsd_max_sp_len ; 

spin_lock_init ( & ifmgd -> teardown_lock ) ; 
ifmgd -> teardown_skb = null ; 
ifmgd -> orig_teardown_skb = null ; 
} 

void ieee80211_mgd_setup_link ( struct ieee80211_link_data * link ) 
{ 
struct ieee80211_sub_if_data * sdata = link -> sdata ; 
struct ieee80211_local * local = sdata -> local ; 
unsigned int link_id = link -> link_id ; 

link -> u . mgd . p2p_noa_index = - 1 ; 
link -> u . mgd . conn_flags = 0 ; 
link -> conf -> bssid = link -> u . mgd . bssid ; 

init_work ( & link -> u . mgd . request_smps_work , 
ieee80211_request_smps_mgd_work ) ; 
if ( local -> hw . wiphy -> features & nl80211_feature_dynamic_smps ) 
link -> u . mgd . req_smps = ieee80211_smps_automatic ; 
else 
link -> u . mgd . req_smps = ieee80211_smps_off ; 

init_work ( & link -> u . mgd . chswitch_work , ieee80211_chswitch_work ) ; 
timer_setup ( & link -> u . mgd . chswitch_timer , ieee80211_chswitch_timer , 0 ) ; 

if ( sdata -> u . mgd . assoc_data ) 
ether_addr_copy ( link -> conf -> addr , 
sdata -> u . mgd . assoc_data -> link [ link_id ] . addr ) ; 
else if ( ! is_valid_ether_addr ( link -> conf -> addr ) ) 
eth_random_addr ( link -> conf -> addr ) ; 
} 


void ieee80211_mlme_notify_scan_completed ( struct ieee80211_local * local ) 
{ 
struct ieee80211_sub_if_data * sdata ; 


rcu_read_lock ( ) ; 
list_for_each_entry_rcu ( sdata , & local -> interfaces , list ) { 
if ( ieee80211_sdata_running ( sdata ) ) 
ieee80211_restart_sta_timer ( sdata ) ; 
} 
rcu_read_unlock ( ) ; 
} 

static int ieee80211_prep_connection ( struct ieee80211_sub_if_data * sdata , 
struct cfg80211_bss * cbss , s8 link_id , 
const u8 * ap_mld_addr , bool assoc , 
bool override ) 
{ 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct ieee80211_bss * bss = ( void * ) cbss -> priv ; 
struct sta_info * new_sta = null ; 
struct ieee80211_link_data * link ; 
bool have_sta = false ; 
bool mlo ; 
int err ; 

if ( link_id >= 0 ) { 
mlo = true ; 
if ( warn_on ( ! ap_mld_addr ) ) 
return - einval ; 
err = ieee80211_vif_set_links ( sdata , bit ( link_id ) ) ; 
} else { 
if ( warn_on ( ap_mld_addr ) ) 
return - einval ; 
ap_mld_addr = cbss -> bssid ; 
err = ieee80211_vif_set_links ( sdata , 0 ) ; 
link_id = 0 ; 
mlo = false ; 
} 

if ( err ) 
return err ; 

link = sdata_dereference ( sdata -> link [ link_id ] , sdata ) ; 
if ( warn_on ( ! link ) ) { 
err = - enolink ; 
goto out_err ; 
} 

if ( warn_on ( ! ifmgd -> auth_data && ! ifmgd -> assoc_data ) ) { 
err = - einval ; 
goto out_err ; 
} 


if ( local -> in_reconfig ) { 
err = - ebusy ; 
goto out_err ; 
} 

if ( assoc ) { 
rcu_read_lock ( ) ; 
have_sta = sta_info_get ( sdata , ap_mld_addr ) ; 
rcu_read_unlock ( ) ; 
} 

if ( ! have_sta ) { 
if ( mlo ) 
new_sta = sta_info_alloc_with_link ( sdata , ap_mld_addr , 
link_id , cbss -> bssid , 
gfp_kernel ) ; 
else 
new_sta = sta_info_alloc ( sdata , ap_mld_addr , gfp_kernel ) ; 

if ( ! new_sta ) { 
err = - enomem ; 
goto out_err ; 
} 

new_sta -> sta . mlo = mlo ; 
} 














if ( new_sta ) { 
const struct cfg80211_bss_ies * ies ; 
struct link_sta_info * link_sta ; 

rcu_read_lock ( ) ; 
link_sta = rcu_dereference ( new_sta -> link [ link_id ] ) ; 
if ( warn_on ( ! link_sta ) ) { 
rcu_read_unlock ( ) ; 
sta_info_free ( local , new_sta ) ; 
err = - einval ; 
goto out_err ; 
} 

err = ieee80211_mgd_setup_link_sta ( link , new_sta , 
link_sta , cbss ) ; 
if ( err ) { 
rcu_read_unlock ( ) ; 
sta_info_free ( local , new_sta ) ; 
goto out_err ; 
} 

memcpy ( link -> u . mgd . bssid , cbss -> bssid , eth_alen ) ; 


link -> conf -> beacon_int = cbss -> beacon_interval ; 
ies = rcu_dereference ( cbss -> beacon_ies ) ; 
if ( ies ) { 
link -> conf -> sync_tsf = ies -> tsf ; 
link -> conf -> sync_device_ts = 
bss -> device_ts_beacon ; 

ieee80211_get_dtim ( ies , 
& link -> conf -> sync_dtim_count , 
null ) ; 
} else if ( ! ieee80211_hw_check ( & sdata -> local -> hw , 
timing_beacon_only ) ) { 
ies = rcu_dereference ( cbss -> proberesp_ies ) ; 

link -> conf -> sync_tsf = ies -> tsf ; 
link -> conf -> sync_device_ts = 
bss -> device_ts_presp ; 
link -> conf -> sync_dtim_count = 0 ; 
} else { 
link -> conf -> sync_tsf = 0 ; 
link -> conf -> sync_device_ts = 0 ; 
link -> conf -> sync_dtim_count = 0 ; 
} 
rcu_read_unlock ( ) ; 
} 

if ( new_sta || override ) { 
err = ieee80211_prep_channel ( sdata , link , cbss , 
& link -> u . mgd . conn_flags ) ; 
if ( err ) { 
if ( new_sta ) 
sta_info_free ( local , new_sta ) ; 
goto out_err ; 
} 
} 

if ( new_sta ) { 




ieee80211_link_info_change_notify ( sdata , link , 
bss_changed_bssid | 
bss_changed_basic_rates | 
bss_changed_beacon_int ) ; 

if ( assoc ) 
sta_info_pre_move_state ( new_sta , ieee80211_sta_auth ) ; 

err = sta_info_insert ( new_sta ) ; 
new_sta = null ; 
if ( err ) { 
sdata_info ( sdata , 
"failed to insert sta entry for the ap (error %d)\n" , 
err ) ; 
goto out_err ; 
} 
} else 
warn_on_once ( ! ether_addr_equal ( link -> u . mgd . bssid , cbss -> bssid ) ) ; 


if ( local -> scanning ) 
ieee80211_scan_cancel ( local ) ; 

return 0 ; 

out_err : 
ieee80211_link_release_channel ( & sdata -> deflink ) ; 
ieee80211_vif_set_links ( sdata , 0 ) ; 
return err ; 
} 


int ieee80211_mgd_auth ( struct ieee80211_sub_if_data * sdata , 
struct cfg80211_auth_request * req ) 
{ 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct ieee80211_mgd_auth_data * auth_data ; 
u16 auth_alg ; 
int err ; 
bool cont_auth ; 



switch ( req -> auth_type ) { 
case nl80211_authtype_open_system : 
auth_alg = wlan_auth_open ; 
break ; 
case nl80211_authtype_shared_key : 
if ( fips_enabled ) 
return - eopnotsupp ; 
auth_alg = wlan_auth_shared_key ; 
break ; 
case nl80211_authtype_ft : 
auth_alg = wlan_auth_ft ; 
break ; 
case nl80211_authtype_network_eap : 
auth_alg = wlan_auth_leap ; 
break ; 
case nl80211_authtype_sae : 
auth_alg = wlan_auth_sae ; 
break ; 
case nl80211_authtype_fils_sk : 
auth_alg = wlan_auth_fils_sk ; 
break ; 
case nl80211_authtype_fils_sk_pfs : 
auth_alg = wlan_auth_fils_sk_pfs ; 
break ; 
case nl80211_authtype_fils_pk : 
auth_alg = wlan_auth_fils_pk ; 
break ; 
default : 
return - eopnotsupp ; 
} 

if ( ifmgd -> assoc_data ) 
return - ebusy ; 

auth_data = kzalloc ( sizeof ( * auth_data ) + req -> auth_data_len + 
req -> ie_len , gfp_kernel ) ; 
if ( ! auth_data ) 
return - enomem ; 

memcpy ( auth_data -> ap_addr , 
req -> ap_mld_addr ? : req -> bss -> bssid , 
eth_alen ) ; 
auth_data -> bss = req -> bss ; 
auth_data -> link_id = req -> link_id ; 

if ( req -> auth_data_len >= 4 ) { 
if ( req -> auth_type == nl80211_authtype_sae ) { 
__le16 * pos = ( __le16 * ) req -> auth_data ; 

auth_data -> sae_trans = le16_to_cpu ( pos [ 0 ] ) ; 
auth_data -> sae_status = le16_to_cpu ( pos [ 1 ] ) ; 
} 
memcpy ( auth_data -> data , req -> auth_data + 4 , 
req -> auth_data_len - 4 ) ; 
auth_data -> data_len += req -> auth_data_len - 4 ; 
} 






cont_auth = ifmgd -> auth_data && req -> bss == ifmgd -> auth_data -> bss && 
ifmgd -> auth_data -> link_id == req -> link_id ; 

if ( req -> ie && req -> ie_len ) { 
memcpy ( & auth_data -> data [ auth_data -> data_len ] , 
req -> ie , req -> ie_len ) ; 
auth_data -> data_len += req -> ie_len ; 
} 

if ( req -> key && req -> key_len ) { 
auth_data -> key_len = req -> key_len ; 
auth_data -> key_idx = req -> key_idx ; 
memcpy ( auth_data -> key , req -> key , req -> key_len ) ; 
} 

auth_data -> algorithm = auth_alg ; 



if ( ifmgd -> auth_data ) { 
if ( cont_auth && req -> auth_type == nl80211_authtype_sae ) { 
auth_data -> peer_confirmed = 
ifmgd -> auth_data -> peer_confirmed ; 
} 
ieee80211_destroy_auth_data ( sdata , cont_auth ) ; 
} 


ifmgd -> auth_data = auth_data ; 






if ( cont_auth && req -> auth_type == nl80211_authtype_sae && 
auth_data -> peer_confirmed && auth_data -> sae_trans == 2 ) 
ieee80211_mark_sta_auth ( sdata ) ; 

if ( ifmgd -> associated ) { 
u8 frame_buf [ ieee80211_deauth_frame_len ] ; 

sdata_info ( sdata , 
"disconnect from ap %pm for new auth to %pm\n" , 
sdata -> vif . cfg . ap_addr , auth_data -> ap_addr ) ; 
ieee80211_set_disassoc ( sdata , ieee80211_stype_deauth , 
wlan_reason_unspecified , 
false , frame_buf ) ; 

ieee80211_report_disconnect ( sdata , frame_buf , 
sizeof ( frame_buf ) , true , 
wlan_reason_unspecified , 
false ) ; 
} 

sdata_info ( sdata , "authenticate with %pm\n" , auth_data -> ap_addr ) ; 


memcpy ( sdata -> vif . cfg . ap_addr , auth_data -> ap_addr , eth_alen ) ; 

err = ieee80211_prep_connection ( sdata , req -> bss , req -> link_id , 
req -> ap_mld_addr , cont_auth , false ) ; 
if ( err ) 
goto err_clear ; 

err = ieee80211_auth ( sdata ) ; 
if ( err ) { 
sta_info_destroy_addr ( sdata , auth_data -> ap_addr ) ; 
goto err_clear ; 
} 


cfg80211_ref_bss ( local -> hw . wiphy , auth_data -> bss ) ; 
return 0 ; 

err_clear : 
if ( ! sdata -> vif . valid_links ) { 
eth_zero_addr ( sdata -> deflink . u . mgd . bssid ) ; 
ieee80211_link_info_change_notify ( sdata , & sdata -> deflink , 
bss_changed_bssid ) ; 
mutex_lock ( & sdata -> local -> mtx ) ; 
ieee80211_link_release_channel ( & sdata -> deflink ) ; 
mutex_unlock ( & sdata -> local -> mtx ) ; 
} 
ifmgd -> auth_data = null ; 
kfree ( auth_data ) ; 
return err ; 
} 

static ieee80211_conn_flags_t 
ieee80211_setup_assoc_link ( struct ieee80211_sub_if_data * sdata , 
struct ieee80211_mgd_assoc_data * assoc_data , 
struct cfg80211_assoc_request * req , 
ieee80211_conn_flags_t conn_flags , 
unsigned int link_id ) 
{ 
struct ieee80211_local * local = sdata -> local ; 
const struct cfg80211_bss_ies * beacon_ies ; 
struct ieee80211_supported_band * sband ; 
const struct element * ht_elem , * vht_elem ; 
struct ieee80211_link_data * link ; 
struct cfg80211_bss * cbss ; 
struct ieee80211_bss * bss ; 
bool is_5ghz , is_6ghz ; 

cbss = assoc_data -> link [ link_id ] . bss ; 
if ( warn_on ( ! cbss ) ) 
return 0 ; 

bss = ( void * ) cbss -> priv ; 

sband = local -> hw . wiphy -> bands [ cbss -> channel -> band ] ; 
if ( warn_on ( ! sband ) ) 
return 0 ; 

link = sdata_dereference ( sdata -> link [ link_id ] , sdata ) ; 
if ( warn_on ( ! link ) ) 
return 0 ; 

is_5ghz = cbss -> channel -> band == nl80211_band_5ghz ; 
is_6ghz = cbss -> channel -> band == nl80211_band_6ghz ; 


if ( ! req -> ap_mld_addr ) { 
assoc_data -> supp_rates = bss -> supp_rates ; 
assoc_data -> supp_rates_len = bss -> supp_rates_len ; 
} 


if ( req -> links [ link_id ] . elems_len ) { 
memcpy ( assoc_data -> ie_pos , req -> links [ link_id ] . elems , 
req -> links [ link_id ] . elems_len ) ; 
assoc_data -> link [ link_id ] . elems = assoc_data -> ie_pos ; 
assoc_data -> link [ link_id ] . elems_len = req -> links [ link_id ] . elems_len ; 
assoc_data -> ie_pos += req -> links [ link_id ] . elems_len ; 
} 

rcu_read_lock ( ) ; 
ht_elem = ieee80211_bss_get_elem ( cbss , wlan_eid_ht_operation ) ; 
if ( ht_elem && ht_elem -> datalen >= sizeof ( struct ieee80211_ht_operation ) ) 
assoc_data -> link [ link_id ] . ap_ht_param = 
( ( struct ieee80211_ht_operation * ) ( ht_elem -> data ) ) -> ht_param ; 
else if ( ! is_6ghz ) 
conn_flags |= ieee80211_conn_disable_ht ; 
vht_elem = ieee80211_bss_get_elem ( cbss , wlan_eid_vht_capability ) ; 
if ( vht_elem && vht_elem -> datalen >= sizeof ( struct ieee80211_vht_cap ) ) { 
memcpy ( & assoc_data -> link [ link_id ] . ap_vht_cap , vht_elem -> data , 
sizeof ( struct ieee80211_vht_cap ) ) ; 
} else if ( is_5ghz ) { 
link_info ( link , 
"vht capa missing/short, disabling vht/he/eht\n" ) ; 
conn_flags |= ieee80211_conn_disable_vht | 
ieee80211_conn_disable_he | 
ieee80211_conn_disable_eht ; 
} 
rcu_read_unlock ( ) ; 

link -> u . mgd . beacon_crc_valid = false ; 
link -> u . mgd . dtim_period = 0 ; 
link -> u . mgd . have_beacon = false ; 


if ( ! ( conn_flags & ieee80211_conn_disable_ht ) ) { 
struct ieee80211_sta_ht_cap sta_ht_cap ; 

memcpy ( & sta_ht_cap , & sband -> ht_cap , sizeof ( sta_ht_cap ) ) ; 
ieee80211_apply_htcap_overrides ( sdata , & sta_ht_cap ) ; 
} 

link -> conf -> eht_puncturing = 0 ; 

rcu_read_lock ( ) ; 
beacon_ies = rcu_dereference ( cbss -> beacon_ies ) ; 
if ( beacon_ies ) { 
const struct ieee80211_eht_operation * eht_oper ; 
const struct element * elem ; 
u8 dtim_count = 0 ; 

ieee80211_get_dtim ( beacon_ies , & dtim_count , 
& link -> u . mgd . dtim_period ) ; 

sdata -> deflink . u . mgd . have_beacon = true ; 

if ( ieee80211_hw_check ( & local -> hw , timing_beacon_only ) ) { 
link -> conf -> sync_tsf = beacon_ies -> tsf ; 
link -> conf -> sync_device_ts = bss -> device_ts_beacon ; 
link -> conf -> sync_dtim_count = dtim_count ; 
} 

elem = cfg80211_find_ext_elem ( wlan_eid_ext_multiple_bssid_configuration , 
beacon_ies -> data , beacon_ies -> len ) ; 
if ( elem && elem -> datalen >= 3 ) 
link -> conf -> profile_periodicity = elem -> data [ 2 ] ; 
else 
link -> conf -> profile_periodicity = 0 ; 

elem = cfg80211_find_elem ( wlan_eid_ext_capability , 
beacon_ies -> data , beacon_ies -> len ) ; 
if ( elem && elem -> datalen >= 11 && 
( elem -> data [ 10 ] & wlan_ext_capa11_ema_support ) ) 
link -> conf -> ema_ap = true ; 
else 
link -> conf -> ema_ap = false ; 

elem = cfg80211_find_ext_elem ( wlan_eid_ext_eht_operation , 
beacon_ies -> data , beacon_ies -> len ) ; 
eht_oper = ( const void * ) ( elem -> data + 1 ) ; 

if ( elem && 
ieee80211_eht_oper_size_ok ( ( const void * ) ( elem -> data + 1 ) , 
elem -> datalen - 1 ) && 
( eht_oper -> params & ieee80211_eht_oper_info_present ) && 
( eht_oper -> params & ieee80211_eht_oper_disabled_subchannel_bitmap_present ) ) { 
const struct ieee80211_eht_operation_info * info = 
( void * ) eht_oper -> optional ; 
const u8 * disable_subchannel_bitmap = info -> optional ; 
u16 bitmap ; 

bitmap = get_unaligned_le16 ( disable_subchannel_bitmap ) ; 
if ( cfg80211_valid_disable_subchannel_bitmap ( & bitmap , 
& link -> conf -> chandef ) ) 
ieee80211_handle_puncturing_bitmap ( link , 
eht_oper , 
bitmap , 
null ) ; 
else 
conn_flags |= ieee80211_conn_disable_eht ; 
} 
} 
rcu_read_unlock ( ) ; 

if ( bss -> corrupt_data ) { 
char * corrupt_type = "data" ; 

if ( bss -> corrupt_data & ieee80211_bss_corrupt_beacon ) { 
if ( bss -> corrupt_data & ieee80211_bss_corrupt_probe_resp ) 
corrupt_type = "beacon and probe response" ; 
else 
corrupt_type = "beacon" ; 
} else if ( bss -> corrupt_data & ieee80211_bss_corrupt_probe_resp ) { 
corrupt_type = "probe response" ; 
} 
sdata_info ( sdata , "associating to ap %pm with corrupt %s\n" , 
cbss -> bssid , corrupt_type ) ; 
} 

if ( link -> u . mgd . req_smps == ieee80211_smps_automatic ) { 
if ( sdata -> u . mgd . powersave ) 
link -> smps_mode = ieee80211_smps_dynamic ; 
else 
link -> smps_mode = ieee80211_smps_off ; 
} else { 
link -> smps_mode = link -> u . mgd . req_smps ; 
} 

return conn_flags ; 
} 

int ieee80211_mgd_assoc ( struct ieee80211_sub_if_data * sdata , 
struct cfg80211_assoc_request * req ) 
{ 
unsigned int assoc_link_id = req -> link_id < 0 ? 0 : req -> link_id ; 
struct ieee80211_local * local = sdata -> local ; 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
struct ieee80211_mgd_assoc_data * assoc_data ; 
const struct element * ssid_elem ; 
struct ieee80211_vif_cfg * vif_cfg = & sdata -> vif . cfg ; 
ieee80211_conn_flags_t conn_flags = 0 ; 
struct ieee80211_link_data * link ; 
struct cfg80211_bss * cbss ; 
struct ieee80211_bss * bss ; 
bool override ; 
int i , err ; 
size_t size = sizeof ( * assoc_data ) + req -> ie_len ; 

for ( i = 0 ; i < ieee80211_mld_max_num_links ; i ++ ) 
size += req -> links [ i ] . elems_len ; 


if ( sdata -> u . mgd . use_4addr && req -> link_id >= 0 ) 
return - eopnotsupp ; 

assoc_data = kzalloc ( size , gfp_kernel ) ; 
if ( ! assoc_data ) 
return - enomem ; 

cbss = req -> link_id < 0 ? req -> bss : req -> links [ req -> link_id ] . bss ; 

rcu_read_lock ( ) ; 
ssid_elem = ieee80211_bss_get_elem ( cbss , wlan_eid_ssid ) ; 
if ( ! ssid_elem || ssid_elem -> datalen > sizeof ( assoc_data -> ssid ) ) { 
rcu_read_unlock ( ) ; 
kfree ( assoc_data ) ; 
return - einval ; 
} 
memcpy ( assoc_data -> ssid , ssid_elem -> data , ssid_elem -> datalen ) ; 
assoc_data -> ssid_len = ssid_elem -> datalen ; 
memcpy ( vif_cfg -> ssid , assoc_data -> ssid , assoc_data -> ssid_len ) ; 
vif_cfg -> ssid_len = assoc_data -> ssid_len ; 
rcu_read_unlock ( ) ; 

if ( req -> ap_mld_addr ) { 
for ( i = 0 ; i < ieee80211_mld_max_num_links ; i ++ ) { 
if ( ! req -> links [ i ] . bss ) 
continue ; 
link = sdata_dereference ( sdata -> link [ i ] , sdata ) ; 
if ( link ) 
ether_addr_copy ( assoc_data -> link [ i ] . addr , 
link -> conf -> addr ) ; 
else 
eth_random_addr ( assoc_data -> link [ i ] . addr ) ; 
} 
} else { 
memcpy ( assoc_data -> link [ 0 ] . addr , sdata -> vif . addr , eth_alen ) ; 
} 

assoc_data -> s1g = cbss -> channel -> band == nl80211_band_s1ghz ; 

memcpy ( assoc_data -> ap_addr , 
req -> ap_mld_addr ? : req -> bss -> bssid , 
eth_alen ) ; 

if ( ifmgd -> associated ) { 
u8 frame_buf [ ieee80211_deauth_frame_len ] ; 

sdata_info ( sdata , 
"disconnect from ap %pm for new assoc to %pm\n" , 
sdata -> vif . cfg . ap_addr , assoc_data -> ap_addr ) ; 
ieee80211_set_disassoc ( sdata , ieee80211_stype_deauth , 
wlan_reason_unspecified , 
false , frame_buf ) ; 

ieee80211_report_disconnect ( sdata , frame_buf , 
sizeof ( frame_buf ) , true , 
wlan_reason_unspecified , 
false ) ; 
} 

if ( ifmgd -> auth_data && ! ifmgd -> auth_data -> done ) { 
err = - ebusy ; 
goto err_free ; 
} 

if ( ifmgd -> assoc_data ) { 
err = - ebusy ; 
goto err_free ; 
} 

if ( ifmgd -> auth_data ) { 
bool match ; 


match = ether_addr_equal ( ifmgd -> auth_data -> ap_addr , 
assoc_data -> ap_addr ) && 
ifmgd -> auth_data -> link_id == req -> link_id ; 
ieee80211_destroy_auth_data ( sdata , match ) ; 
} 



bss = ( void * ) cbss -> priv ; 
assoc_data -> wmm = bss -> wmm_used && 
( local -> hw . queues >= ieee80211_num_acs ) ; 








for ( i = 0 ; i < req -> crypto . n_ciphers_pairwise ; i ++ ) { 
if ( req -> crypto . ciphers_pairwise [ i ] == wlan_cipher_suite_wep40 || 
req -> crypto . ciphers_pairwise [ i ] == wlan_cipher_suite_tkip || 
req -> crypto . ciphers_pairwise [ i ] == wlan_cipher_suite_wep104 ) { 
conn_flags |= ieee80211_conn_disable_ht ; 
conn_flags |= ieee80211_conn_disable_vht ; 
conn_flags |= ieee80211_conn_disable_he ; 
conn_flags |= ieee80211_conn_disable_eht ; 
netdev_info ( sdata -> dev , 
"disabling ht/vht/he due to wep/tkip use\n" ) ; 
} 
} 


if ( ! bss -> wmm_used ) { 
conn_flags |= ieee80211_conn_disable_ht ; 
conn_flags |= ieee80211_conn_disable_vht ; 
conn_flags |= ieee80211_conn_disable_he ; 
conn_flags |= ieee80211_conn_disable_eht ; 
netdev_info ( sdata -> dev , 
"disabling ht/vht/he as wmm/qos is not supported by the ap\n" ) ; 
} 

if ( req -> flags & assoc_req_disable_ht ) { 
mlme_dbg ( sdata , "ht disabled by flag, disabling ht/vht/he\n" ) ; 
conn_flags |= ieee80211_conn_disable_ht ; 
conn_flags |= ieee80211_conn_disable_vht ; 
conn_flags |= ieee80211_conn_disable_he ; 
conn_flags |= ieee80211_conn_disable_eht ; 
} 

if ( req -> flags & assoc_req_disable_vht ) { 
mlme_dbg ( sdata , "vht disabled by flag, disabling vht\n" ) ; 
conn_flags |= ieee80211_conn_disable_vht ; 
} 

if ( req -> flags & assoc_req_disable_he ) { 
mlme_dbg ( sdata , "he disabled by flag, disabling he/eht\n" ) ; 
conn_flags |= ieee80211_conn_disable_he ; 
conn_flags |= ieee80211_conn_disable_eht ; 
} 

if ( req -> flags & assoc_req_disable_eht ) 
conn_flags |= ieee80211_conn_disable_eht ; 

memcpy ( & ifmgd -> ht_capa , & req -> ht_capa , sizeof ( ifmgd -> ht_capa ) ) ; 
memcpy ( & ifmgd -> ht_capa_mask , & req -> ht_capa_mask , 
sizeof ( ifmgd -> ht_capa_mask ) ) ; 

memcpy ( & ifmgd -> vht_capa , & req -> vht_capa , sizeof ( ifmgd -> vht_capa ) ) ; 
memcpy ( & ifmgd -> vht_capa_mask , & req -> vht_capa_mask , 
sizeof ( ifmgd -> vht_capa_mask ) ) ; 

memcpy ( & ifmgd -> s1g_capa , & req -> s1g_capa , sizeof ( ifmgd -> s1g_capa ) ) ; 
memcpy ( & ifmgd -> s1g_capa_mask , & req -> s1g_capa_mask , 
sizeof ( ifmgd -> s1g_capa_mask ) ) ; 

if ( req -> ie && req -> ie_len ) { 
memcpy ( assoc_data -> ie , req -> ie , req -> ie_len ) ; 
assoc_data -> ie_len = req -> ie_len ; 
assoc_data -> ie_pos = assoc_data -> ie + assoc_data -> ie_len ; 
} else { 
assoc_data -> ie_pos = assoc_data -> ie ; 
} 

if ( req -> fils_kek ) { 

if ( warn_on ( req -> fils_kek_len > fils_max_kek_len ) ) { 
err = - einval ; 
goto err_free ; 
} 
memcpy ( assoc_data -> fils_kek , req -> fils_kek , 
req -> fils_kek_len ) ; 
assoc_data -> fils_kek_len = req -> fils_kek_len ; 
} 

if ( req -> fils_nonces ) 
memcpy ( assoc_data -> fils_nonces , req -> fils_nonces , 
2 * fils_nonce_len ) ; 


assoc_data -> timeout = jiffies ; 
assoc_data -> timeout_started = true ; 

assoc_data -> assoc_link_id = assoc_link_id ; 

if ( req -> ap_mld_addr ) { 
for ( i = 0 ; i < array_size ( assoc_data -> link ) ; i ++ ) { 
assoc_data -> link [ i ] . conn_flags = conn_flags ; 
assoc_data -> link [ i ] . bss = req -> links [ i ] . bss ; 
} 


err = ieee80211_vif_set_links ( sdata , bit ( assoc_link_id ) ) ; 
if ( err ) 
goto err_clear ; 
} else { 
assoc_data -> link [ 0 ] . conn_flags = conn_flags ; 
assoc_data -> link [ 0 ] . bss = cbss ; 
} 

link = sdata_dereference ( sdata -> link [ assoc_link_id ] , sdata ) ; 
if ( warn_on ( ! link ) ) { 
err = - einval ; 
goto err_clear ; 
} 


conn_flags |= link -> u . mgd . conn_flags ; 
conn_flags |= ieee80211_setup_assoc_link ( sdata , assoc_data , req , 
conn_flags , assoc_link_id ) ; 
override = link -> u . mgd . conn_flags != conn_flags ; 
link -> u . mgd . conn_flags |= conn_flags ; 

if ( warn ( ( sdata -> vif . driver_flags & ieee80211_vif_supports_uapsd ) && 
ieee80211_hw_check ( & local -> hw , ps_nullfunc_stack ) , 
"u-apsd not supported with hw_ps_nullfunc_stack\n" ) ) 
sdata -> vif . driver_flags &= ~ ieee80211_vif_supports_uapsd ; 

if ( bss -> wmm_used && bss -> uapsd_supported && 
( sdata -> vif . driver_flags & ieee80211_vif_supports_uapsd ) ) { 
assoc_data -> uapsd = true ; 
ifmgd -> flags |= ieee80211_sta_uapsd_enabled ; 
} else { 
assoc_data -> uapsd = false ; 
ifmgd -> flags &= ~ ieee80211_sta_uapsd_enabled ; 
} 

if ( req -> prev_bssid ) 
memcpy ( assoc_data -> prev_ap_addr , req -> prev_bssid , eth_alen ) ; 

if ( req -> use_mfp ) { 
ifmgd -> mfp = ieee80211_mfp_required ; 
ifmgd -> flags |= ieee80211_sta_mfp_enabled ; 
} else { 
ifmgd -> mfp = ieee80211_mfp_disabled ; 
ifmgd -> flags &= ~ ieee80211_sta_mfp_enabled ; 
} 

if ( req -> flags & assoc_req_use_rrm ) 
ifmgd -> flags |= ieee80211_sta_enable_rrm ; 
else 
ifmgd -> flags &= ~ ieee80211_sta_enable_rrm ; 

if ( req -> crypto . control_port ) 
ifmgd -> flags |= ieee80211_sta_control_port ; 
else 
ifmgd -> flags &= ~ ieee80211_sta_control_port ; 

sdata -> control_port_protocol = req -> crypto . control_port_ethertype ; 
sdata -> control_port_no_encrypt = req -> crypto . control_port_no_encrypt ; 
sdata -> control_port_over_nl80211 = 
req -> crypto . control_port_over_nl80211 ; 
sdata -> control_port_no_preauth = req -> crypto . control_port_no_preauth ; 


ifmgd -> assoc_data = assoc_data ; 

for ( i = 0 ; i < array_size ( assoc_data -> link ) ; i ++ ) { 
if ( ! assoc_data -> link [ i ] . bss ) 
continue ; 
if ( i == assoc_data -> assoc_link_id ) 
continue ; 

err = ieee80211_prep_channel ( sdata , null , assoc_data -> link [ i ] . bss , 
& assoc_data -> link [ i ] . conn_flags ) ; 
if ( err ) 
goto err_clear ; 
} 


memcpy ( sdata -> vif . cfg . ap_addr , assoc_data -> ap_addr , eth_alen ) ; 

err = ieee80211_prep_connection ( sdata , cbss , req -> link_id , 
req -> ap_mld_addr , true , override ) ; 
if ( err ) 
goto err_clear ; 

assoc_data -> link [ assoc_data -> assoc_link_id ] . conn_flags = 
link -> u . mgd . conn_flags ; 

if ( ieee80211_hw_check ( & sdata -> local -> hw , need_dtim_before_assoc ) ) { 
const struct cfg80211_bss_ies * beacon_ies ; 

rcu_read_lock ( ) ; 
beacon_ies = rcu_dereference ( req -> bss -> beacon_ies ) ; 

if ( beacon_ies ) { 




sdata_info ( sdata , "waiting for beacon from %pm\n" , 
link -> u . mgd . bssid ) ; 
assoc_data -> timeout = tu_to_exp_time ( req -> bss -> beacon_interval ) ; 
assoc_data -> timeout_started = true ; 
assoc_data -> need_beacon = true ; 
} 
rcu_read_unlock ( ) ; 
} 

run_again ( sdata , assoc_data -> timeout ) ; 

return 0 ; 
err_clear : 
eth_zero_addr ( sdata -> deflink . u . mgd . bssid ) ; 
ieee80211_link_info_change_notify ( sdata , & sdata -> deflink , 
bss_changed_bssid ) ; 
ifmgd -> assoc_data = null ; 
err_free : 
kfree ( assoc_data ) ; 
return err ; 
} 

int ieee80211_mgd_deauth ( struct ieee80211_sub_if_data * sdata , 
struct cfg80211_deauth_request * req ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 
u8 frame_buf [ ieee80211_deauth_frame_len ] ; 
bool tx = ! req -> local_state_change ; 
struct ieee80211_prep_tx_info info = { 
. subtype = ieee80211_stype_deauth , 
} ; 

if ( ifmgd -> auth_data && 
ether_addr_equal ( ifmgd -> auth_data -> ap_addr , req -> bssid ) ) { 
sdata_info ( sdata , 
"aborting authentication with %pm by local choice (reason: %u=%s)\n" , 
req -> bssid , req -> reason_code , 
ieee80211_get_reason_code_string ( req -> reason_code ) ) ; 

drv_mgd_prepare_tx ( sdata -> local , sdata , & info ) ; 
ieee80211_send_deauth_disassoc ( sdata , req -> bssid , req -> bssid , 
ieee80211_stype_deauth , 
req -> reason_code , tx , 
frame_buf ) ; 
ieee80211_destroy_auth_data ( sdata , false ) ; 
ieee80211_report_disconnect ( sdata , frame_buf , 
sizeof ( frame_buf ) , true , 
req -> reason_code , false ) ; 
drv_mgd_complete_tx ( sdata -> local , sdata , & info ) ; 
return 0 ; 
} 

if ( ifmgd -> assoc_data && 
ether_addr_equal ( ifmgd -> assoc_data -> ap_addr , req -> bssid ) ) { 
sdata_info ( sdata , 
"aborting association with %pm by local choice (reason: %u=%s)\n" , 
req -> bssid , req -> reason_code , 
ieee80211_get_reason_code_string ( req -> reason_code ) ) ; 

drv_mgd_prepare_tx ( sdata -> local , sdata , & info ) ; 
ieee80211_send_deauth_disassoc ( sdata , req -> bssid , req -> bssid , 
ieee80211_stype_deauth , 
req -> reason_code , tx , 
frame_buf ) ; 
ieee80211_destroy_assoc_data ( sdata , assoc_abandon ) ; 
ieee80211_report_disconnect ( sdata , frame_buf , 
sizeof ( frame_buf ) , true , 
req -> reason_code , false ) ; 
return 0 ; 
} 

if ( ifmgd -> associated && 
ether_addr_equal ( sdata -> vif . cfg . ap_addr , req -> bssid ) ) { 
sdata_info ( sdata , 
"deauthenticating from %pm by local choice (reason: %u=%s)\n" , 
req -> bssid , req -> reason_code , 
ieee80211_get_reason_code_string ( req -> reason_code ) ) ; 

ieee80211_set_disassoc ( sdata , ieee80211_stype_deauth , 
req -> reason_code , tx , frame_buf ) ; 
ieee80211_report_disconnect ( sdata , frame_buf , 
sizeof ( frame_buf ) , true , 
req -> reason_code , false ) ; 
drv_mgd_complete_tx ( sdata -> local , sdata , & info ) ; 
return 0 ; 
} 

return - enotconn ; 
} 

int ieee80211_mgd_disassoc ( struct ieee80211_sub_if_data * sdata , 
struct cfg80211_disassoc_request * req ) 
{ 
u8 frame_buf [ ieee80211_deauth_frame_len ] ; 

if ( ! sdata -> u . mgd . associated || 
memcmp ( sdata -> vif . cfg . ap_addr , req -> ap_addr , eth_alen ) ) 
return - enotconn ; 

sdata_info ( sdata , 
"disassociating from %pm by local choice (reason: %u=%s)\n" , 
req -> ap_addr , req -> reason_code , 
ieee80211_get_reason_code_string ( req -> reason_code ) ) ; 

ieee80211_set_disassoc ( sdata , ieee80211_stype_disassoc , 
req -> reason_code , ! req -> local_state_change , 
frame_buf ) ; 

ieee80211_report_disconnect ( sdata , frame_buf , sizeof ( frame_buf ) , true , 
req -> reason_code , false ) ; 

return 0 ; 
} 

void ieee80211_mgd_stop_link ( struct ieee80211_link_data * link ) 
{ 
cancel_work_sync ( & link -> u . mgd . request_smps_work ) ; 
cancel_work_sync ( & link -> u . mgd . chswitch_work ) ; 
} 

void ieee80211_mgd_stop ( struct ieee80211_sub_if_data * sdata ) 
{ 
struct ieee80211_if_managed * ifmgd = & sdata -> u . mgd ; 






cancel_work_sync ( & ifmgd -> monitor_work ) ; 
cancel_work_sync ( & ifmgd -> beacon_connection_loss_work ) ; 
cancel_work_sync ( & ifmgd -> csa_connection_drop_work ) ; 
cancel_delayed_work_sync ( & ifmgd -> tdls_peer_del_work ) ; 

sdata_lock ( sdata ) ; 
if ( ifmgd -> assoc_data ) 
ieee80211_destroy_assoc_data ( sdata , assoc_timeout ) ; 
if ( ifmgd -> auth_data ) 
ieee80211_destroy_auth_data ( sdata , false ) ; 
spin_lock_bh ( & ifmgd -> teardown_lock ) ; 
if ( ifmgd -> teardown_skb ) { 
kfree_skb ( ifmgd -> teardown_skb ) ; 
ifmgd -> teardown_skb = null ; 
ifmgd -> orig_teardown_skb = null ; 
} 
kfree ( ifmgd -> assoc_req_ies ) ; 
ifmgd -> assoc_req_ies = null ; 
ifmgd -> assoc_req_ies_len = 0 ; 
spin_unlock_bh ( & ifmgd -> teardown_lock ) ; 
del_timer_sync ( & ifmgd -> timer ) ; 
sdata_unlock ( sdata ) ; 
} 

void ieee80211_cqm_rssi_notify ( struct ieee80211_vif * vif , 
enum nl80211_cqm_rssi_threshold_event rssi_event , 
s32 rssi_level , 
gfp_t gfp ) 
{ 
struct ieee80211_sub_if_data * sdata = vif_to_sdata ( vif ) ; 

trace_api_cqm_rssi_notify ( sdata , rssi_event , rssi_level ) ; 

cfg80211_cqm_rssi_notify ( sdata -> dev , rssi_event , rssi_level , gfp ) ; 
} 
export_symbol ( ieee80211_cqm_rssi_notify ) ; 

void ieee80211_cqm_beacon_loss_notify ( struct ieee80211_vif * vif , gfp_t gfp ) 
{ 
struct ieee80211_sub_if_data * sdata = vif_to_sdata ( vif ) ; 

trace_api_cqm_beacon_loss_notify ( sdata -> local , sdata ) ; 

cfg80211_cqm_beacon_loss_notify ( sdata -> dev , gfp ) ; 
} 
export_symbol ( ieee80211_cqm_beacon_loss_notify ) ; 

static void _ieee80211_enable_rssi_reports ( struct ieee80211_sub_if_data * sdata , 
int rssi_min_thold , 
int rssi_max_thold ) 
{ 
trace_api_enable_rssi_reports ( sdata , rssi_min_thold , rssi_max_thold ) ; 

if ( warn_on ( sdata -> vif . type != nl80211_iftype_station ) ) 
return ; 






sdata -> u . mgd . rssi_min_thold = rssi_min_thold * 16 ; 
sdata -> u . mgd . rssi_max_thold = rssi_max_thold * 16 ; 
} 

void ieee80211_enable_rssi_reports ( struct ieee80211_vif * vif , 
int rssi_min_thold , 
int rssi_max_thold ) 
{ 
struct ieee80211_sub_if_data * sdata = vif_to_sdata ( vif ) ; 

warn_on ( rssi_min_thold == rssi_max_thold || 
rssi_min_thold > rssi_max_thold ) ; 

_ieee80211_enable_rssi_reports ( sdata , rssi_min_thold , 
rssi_max_thold ) ; 
} 
export_symbol ( ieee80211_enable_rssi_reports ) ; 

void ieee80211_disable_rssi_reports ( struct ieee80211_vif * vif ) 
{ 
struct ieee80211_sub_if_data * sdata = vif_to_sdata ( vif ) ; 

_ieee80211_enable_rssi_reports ( sdata , 0 , 0 ) ; 
} 
export_symbol ( ieee80211_disable_rssi_reports ) ; 
