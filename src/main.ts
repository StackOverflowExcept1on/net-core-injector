import yargs from "yargs";
import * as frida from "frida";
import * as fs from "fs";
import * as path from "path";

yargs(process.argv.slice(2))
    .scriptName("net-core-injector")
    .usage("$0 <cmd> <args>")
    .command("inject <process_name> <bootstrapper> <runtime_config_path> <assembly_path> <type_name> <method_name>", "inject C# library into process", (yargs) => {
        yargs
            .positional("process_name", {type: "string"})
            .positional("bootstrapper", {type: "string"})
            .positional("runtime_config_path", {type: "string"})
            .positional("assembly_path", {type: "string"})
            .positional("type_name", {type: "string"})
            .positional("method_name", {type: "string"})
    }, async (argv: any) => {
        const session = await frida.attach(argv.process_name);

        const source = fs.readFileSync("dist/agent.js", "utf8");

        const script = await session.createScript(source);
        await script.load();

        const api: any = script.exports;
        const ret = await api.inject(
            path.resolve(argv.bootstrapper),
            path.resolve(argv.runtime_config_path),
            path.resolve(argv.assembly_path),
            argv.type_name,
            argv.method_name,
        );
        console.log(`[*] api.inject() => ${ret} (InitializeResult return value)`);
        if (ret !== 0) {
            console.log(`An error occurred while injection into ${argv.process_name}, see InitializeResult in sources`);
        }
        await script.unload();
    })
    .demandCommand(1)
    .help()
    .argv;
