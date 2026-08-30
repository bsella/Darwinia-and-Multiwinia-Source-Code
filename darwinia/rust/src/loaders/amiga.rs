use crate::loaders::Loader;

pub struct AmigaLoader {}

impl Loader for AmigaLoader {
    const NAME: &std::ffi::CStr = c"Amiga";
    const CHANCE_OF_SPAWNING : u32 = 10;
}
