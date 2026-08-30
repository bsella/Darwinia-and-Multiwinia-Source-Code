use crate::loaders::Loader;

pub struct FodderLoader {}

impl Loader for FodderLoader {
    const NAME: &std::ffi::CStr = c"Fodder";
    const CHANCE_OF_SPAWNING : u32 = 10;
}
