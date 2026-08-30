use crate::loaders::Loader;

pub struct MatrixLoader {}

impl Loader for MatrixLoader {
    const NAME: &std::ffi::CStr = c"Matrix";
    const CHANCE_OF_SPAWNING : u32 = 10;
}
