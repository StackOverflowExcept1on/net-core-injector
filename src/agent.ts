rpc.exports = {
    inject: (bootstrapper: string, runtime_config_path: string, assembly_path: string, type_name: string, method_name: string): number => {
        const bootstrapperModule = Module.load(bootstrapper);

        const functionPointer = bootstrapperModule.getExportByName("bootstrapper_load_assembly");
        const bootstrapper_load_assembly = new NativeFunction(functionPointer, "uint32", ["pointer", "pointer", "pointer", "pointer"]);

        const allocUtfString = Process.platform === "windows" ? Memory.allocUtf16String : Memory.allocUtf8String;
        return bootstrapper_load_assembly(
            allocUtfString(runtime_config_path),
            allocUtfString(assembly_path),
            allocUtfString(type_name),
            allocUtfString(method_name),
        );
    },
};
