use crate::loaders::Loader;

pub struct SoulLoader {}

impl Loader for SoulLoader {
    const NAME: &std::ffi::CStr = c"Soul";
    const CHANCE_OF_SPAWNING : u32 = 10;
}
