import {decode as decodeMsgpack} from "@msgpack/msgpack";

import type {QueryJobsItem} from "./sql";

type CompressionJobConfig = {
    input: {
        paths_to_compress: Array<string>;
        path_prefix_to_remove: string;
        dataset?: string;
        timestamp_key?: string;
    };
    output: Record<string, unknown>;
};

type DecodedQueryJobsItem = QueryJobsItem & {
    dataset: string | null;
    paths: string[];
};

type BufferLike = {
    type: string;
    data: Array<number>;
};

/**
 * Extracts the raw bytes out of the JSON serialized MySQL varbinary column.
 *
 * @param clpConfig
 * @return
 */
const toUint8Array = (clpConfig: unknown): Uint8Array | null => {
    if (clpConfig instanceof Uint8Array) {
        return clpConfig;
    }
    if (clpConfig instanceof ArrayBuffer) {
        return new Uint8Array(clpConfig);
    }
    if ("object" === typeof clpConfig &&
        null !== clpConfig &&
        "Buffer" === (clpConfig as BufferLike).type &&
        Array.isArray((clpConfig as BufferLike).data)
    ) {
        return new Uint8Array((clpConfig as BufferLike).data);
    }

    return null;
};

/**
 * Decompresses a brotli-compressed Uint8Array using the browser's built-in
 * DecompressionStream. Returns null if unsupported or on error.
 *
 * @param data
 * @return
 */
const decompressBrotli = async (data: Uint8Array): Promise<Uint8Array | null> => {
    const DecompressionStreamCtor =
        (globalThis as unknown as {DecompressionStream?: typeof DecompressionStream}).DecompressionStream;
    if ("function" !== typeof DecompressionStreamCtor) {
        console.error("Brotli decompression is not supported in this browser");
        return null;
    }

    try {
        const stream = new Blob([data]).stream().pipeThrough(
            new DecompressionStreamCtor("brotli")
        );
        const decompressed = await new Response(stream).arrayBuffer();
        return new Uint8Array(decompressed);
    } catch (err: unknown) {
        console.error("Failed to brotli decompress job config", err);
        return null;
    }
};

/**
 * Turns a compressed job config blob into a decoded config object.
 *
 * @param clpConfig
 * @return
 */
const decodeJobConfigBytes = async (
    clpConfig: unknown
): Promise<CompressionJobConfig | null> => {
    const buffer = toUint8Array(clpConfig);
    if (null === buffer) return null;

    const decompressed = await decompressBrotli(buffer);
    if (null === decompressed) return null;

    try {
        const decoded = decodeMsgpack(decompressed);
        if ("object" !== typeof decoded || null === decoded) return null;

        return decoded as CompressionJobConfig;
    } catch (err: unknown) {
        console.error("Failed to decode compression job config", err);
        return null;
    }
};

/**
 * Decodes the job config into dataset and user paths.
 *
 * @param item
 * @return
 */
const decodeJobConfig = async (item: QueryJobsItem): Promise<DecodedQueryJobsItem> => {
    const decoded = await decodeJobConfigBytes(item.clp_config);
    if (null === decoded) {
        return {
            ...item,
            dataset: null,
            paths: [],
        };
    }

    const input = decoded.input ?? {};
    const pathsToCompress = Array.isArray(input.paths_to_compress)
        ? input.paths_to_compress
        : [];

    const paths = pathsToCompress
        .filter((path): path is string => "string" === typeof path)
        .map((path) => {
            if ("string" === typeof input.path_prefix_to_remove &&
                path.startsWith(input.path_prefix_to_remove)
            ) {
                return path.slice(input.path_prefix_to_remove.length);
            }
            return path;
        });

    return {
        ...item,
        dataset: "string" === typeof input.dataset ? input.dataset : null,
        paths,
    };
};


export type {DecodedQueryJobsItem};
export {decodeJobConfig};
