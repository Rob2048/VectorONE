from distutils.core import setup, Extension

setup (
	ext_modules = [
		Extension(
			'blobdetect',
	 		sources = ['blobdetectmodule.cpp'],
	 		extra_compile_args = ['--std=c++11'],
	 		language='c++11',
	 	),
	 ]
)