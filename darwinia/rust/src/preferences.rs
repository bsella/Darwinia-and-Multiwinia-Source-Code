use std::ffi::c_char;

unsafe extern "C" {
    pub fn darw_PreferenceManager_GetString(
        key: *const c_char,
        def: *const c_char,
    ) -> *const c_char;
    pub fn darw_PreferenceManager_GetFloat(key: *const c_char, def: f32) -> f32;
    pub fn darw_PreferenceManager_GetInt(key: *const c_char, def: i32) -> i32;
    pub fn darw_PreferenceManager_SetString(key: *const c_char, val: *const c_char);
    pub fn darw_PreferenceManager_SetFloat(key: *const c_char, val: f32);
    pub fn darw_PreferenceManager_SetInt(key: *const c_char, val: i32);
}
