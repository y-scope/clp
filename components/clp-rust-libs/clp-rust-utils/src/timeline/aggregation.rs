//! Aggregation of timeline archive buckets stored in MongoDB.

use mongodb::Collection;
use mongodb::bson::Document;
use mongodb::bson::doc;

use super::COMMIT_MARKER_ID;

/// Combines archive buckets, excluding already published buckets and the commit marker.
///
/// Integer identities are reserved for final buckets. Spider archive buckets use object identities.
/// Invalid remaining identities and operands are retained and flagged rather than silently
/// discarded.
///
/// # Returns
///
/// A cursor of grouped, timestamp-ordered documents on success. Each document must be validated
/// with [`super::Bucket::try_from_aggregation_document`] before publication or use.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`Collection::aggregate`]'s return values on failure.
pub async fn merge_archive_buckets(
    collection: &Collection<Document>,
) -> mongodb::error::Result<mongodb::Cursor<Document>> {
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
    collection.aggregate(pipeline).await
}
