//! Aggregation of timeline archive buckets stored in MongoDB.

use futures::Stream;
use futures::StreamExt;
use mongodb::Collection;
use mongodb::bson::Document;
use mongodb::bson::doc;

use super::Bucket;
use super::COMMIT_MARKER_ID;
use super::Error;

/// Combines archive buckets, excluding already published buckets and the commit marker.
///
/// Integer identities are reserved for final buckets. Spider archive buckets use object identities.
/// Invalid remaining identities and operands are retained and flagged rather than silently
/// discarded.
///
/// # Returns
///
/// A stream of validated, timestamp-ordered buckets on success. The stream yields an error if
/// reading or validating a bucket fails.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`Collection::aggregate`]'s return values on failure.
///
/// The returned stream yields [`Error::Mongo`] if reading a bucket fails and forwards
/// [`Bucket::try_from_aggregation_document`]'s errors.
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
