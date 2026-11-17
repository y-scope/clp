//! Aggregation of timeline archive buckets stored in MongoDB.

use futures::Stream;
use futures::StreamExt;
use mongodb::Collection;
use mongodb::bson::Document;
use mongodb::bson::doc;

use super::Bucket;
use super::COMMIT_MARKER_ID;
use super::Error;

/// Merges per-archive timeline buckets into final buckets, excluding already published final
/// buckets and the commit marker.
///
/// # Returns
///
/// A stream of the merged buckets in ascending timestamp order on success. Each item in the stream
/// is a [`Result`] that:
///
/// ## Returns
///
/// A validated bucket on success.
///
/// ## Errors
///
/// Returns an error if:
///
/// * [`Error::Mongo`] if reading the next document from the cursor fails.
/// * Forwards [`Bucket::try_from_aggregation_document`]'s return values on failure.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`Collection::aggregate`]'s return values on failure.
pub async fn merge_archive_buckets(
    collection: &Collection<Document>,
) -> Result<impl Stream<Item = Result<Bucket, Error>> + Unpin, Error> {
    let pipeline = [
        doc! {
            "$match": { "$expr": { "$and": [
                { "$not": [{ "$in": [{ "$type": "$_id" }, ["int", "long"]] }] },
                { "$ne": ["$_id", COMMIT_MARKER_ID] },
            ] } },
        },
        doc! {
            "$project": {
                "timestamp": "$_id.timestamp",
                "valid_identity": { "$eq": [{ "$type": "$_id" }, "object"] },
                "count": 1,
            },
        },
        doc! {
            "$group": {
                "_id": "$timestamp",
                "count": { "$sum": "$count" },
                "invalid": { "$max": { "$cond": [
                    { "$and": [
                        "$valid_identity",
                        { "$in": [{ "$type": "$timestamp" }, ["int", "long"]] },
                        { "$in": [{ "$type": "$count" }, ["int", "long"]] },
                        { "$gte": ["$count", 0] },
                    ] },
                    0,
                    1,
                ] } },
            },
        },
        doc! { "$sort": { "_id": 1 } },
    ];
    let cursor = collection.aggregate(pipeline).await?;
    Ok(cursor.map(|document| {
        let document = document?;
        Bucket::try_from_aggregation_document(&document)
    }))
}
