import axios from "axios";


type Settings = {
    ClpStorageEngine: string;
    ClpQueryEngine: string;
    LogsInputType: string;
    LogsInputRootDir: string | null;
    MaxDatasetsPerQuery: number | null;
    MongoDbSearchResultsMetadataCollectionName: string;
    SqlDbClpArchivesTableName: string;
    SqlDbClpDatasetsTableName: string;
    SqlDbClpFilesTableName: string;
    SqlDbClpTablePrefix: string;
    SqlDbCompressionJobsTableName: string;
};

/**
 * Loads application settings from a `settings.json` file.
 *
 * This function fetches a JSON file named `settings.json` from the server
 * and parses its contents into a `Settings` object.
 *
 * @return
 * @throws {Error} If the fetch or JSON parsing fails, an error is thrown with the original cause.
 */
const loadSettings = async (): Promise<Settings> => {
    try {
        const response = await axios.get<Settings>("settings.json");
        return response.data;
    } catch (e: unknown) {
        throw new Error("Failed to fetch settings.", {cause: e});
    }
};

const settings: Settings = await loadSettings();


export {settings};
