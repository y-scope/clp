import {
    CLP_STORAGE_ENGINES,
    STORAGE_TYPE,
} from "@webui/common/config";
import tap from "tap";

import {enabledSettings} from "./s3-scanner.helpers.js";


tap.test("public scanner availability follows the URL and deployment", async (t) => {
    for (const clpS of [false,
        true]) {
        for (const s3 of [false,
            true]) {
            for (const url of [null,
                "http://ingestor.internal:3002"]) {
                const source = enabledSettings();
                source.public.ClpStorageEngine = clpS ?
                    CLP_STORAGE_ENGINES.CLP_S :
                    CLP_STORAGE_ENGINES.CLP;
                source.public.LogsInputType = s3 ?
                    STORAGE_TYPE.S3 :
                    STORAGE_TYPE.FS;
                source.server.LogIngestorUrl = url;
                const expected = clpS && s3 && null !== url;
                const raw = {
                    ...source,
                    public: {...source.public, S3ScannerEnabled: !expected},
                };
                const {publicSettings, serverSettings} = await t.mockImport<
                    typeof import("../settings.js")
                >("../settings.js", {
                    "node:fs": {
                        existsSync: () => true,
                        readFileSync: () => JSON.stringify(raw),
                    },
                });

                t.equal(publicSettings.S3ScannerEnabled, expected);
                t.notOk(Object.hasOwn(publicSettings, "LogIngestorUrl"));
                t.equal(serverSettings.LogIngestorUrl, url);
            }
        }
    }
});

tap.test("source-layout settings use the same derived availability", async (t) => {
    let probes = 0;
    const source = enabledSettings();
    source.server.LogIngestorUrl = null;
    const {publicSettings} = await t.mockImport<typeof import("../settings.js")>("../settings.js", {
        "node:fs": {
            existsSync: () => 1 < ++probes,
            readFileSync: () => JSON.stringify(source),
        },
    });

    t.equal(publicSettings.S3ScannerEnabled, false);
});

tap.test("missing settings still fail startup", async (t) => {
    await t.rejects(t.mockImport("../settings.js", {
        "node:fs": {existsSync: () => false, readFileSync: () => ""},
    }), {message: /Unable to locate the WebUI settings file/});
});
