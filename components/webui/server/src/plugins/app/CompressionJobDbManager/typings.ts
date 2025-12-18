interface CompressionJobConfig {
    input: {
        paths_to_compress: string[];
        path_prefix_to_remove: string;
        dataset?: string;
        timestamp_key?: string;
        unstructured?: boolean;
    };
    output: {
        target_archive_size: number;
        target_dictionaries_size: number;
        target_segment_size: number;
        target_encoded_file_size: number;
        compression_level: number;
    };
}


export type {CompressionJobConfig};
