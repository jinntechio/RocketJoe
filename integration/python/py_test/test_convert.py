import pytest
from ottergon import to_aggregate


def test_convert_aggregate_match():
    example = [
        {
            "$match": {
                "size": "medium"
            }
        }
    ]
    assert to_aggregate(example) == '$aggregate: {$match: {"size": {$eq: #0}}}'

    example = [
        {
            "$match": {
                "size": "medium",
                "count": {
                    "$lt": 10
                },
                "name": {
                    "$regex": "N*"
                }
            }
        }
    ]
    assert to_aggregate(example) == '$aggregate: {$match: {$and: [{"size": {$eq: #0}}, {"count": {$lt: #1}}, ' \
                                    '{"name": {$regex: #2}}]}}'


def test_convert_aggregate_group():
    example = [
        {
            "$group": {
                "_id": "name"
            }
        }
    ]
    assert to_aggregate(example) == '$aggregate: {$group: {_id: #0}}'

    example = [
        {
            "$group": {
                "_id": "$name"
            }
        }
    ]
    assert to_aggregate(example) == '$aggregate: {$group: {_id: "$name"}}'

    example = [
        {
            "$group": {
                "sum": {
                    "$sum": "$count"
                }
            }
        }
    ]
    assert to_aggregate(example) == '$aggregate: {$group: {sum: {$sum: "$count"}}}'

    example = [
        {
            "$group": {
                "total": {
                    "$multiply": [
                        "$price",
                        "$count"
                    ]
                }
            }
        }
    ]
    assert to_aggregate(example) == '$aggregate: {$group: {total: {$multiply: ["$price", "$count"]}}}'

    example = [
        {
            "$group": {
                "total": {
                    "$multiply": [
                        "$price",
                        10
                    ]
                }
            }
        }
    ]
    assert to_aggregate(example) == '$aggregate: {$group: {total: {$multiply: ["$price", #0]}}}'

    example = [
        {
            "$group": {
                "_id": "$name",
                "sum": {
                    "$sum": "$count"
                }
            }
        }
    ]
    assert to_aggregate(example) == '$aggregate: {$group: {_id: "$name", sum: {$sum: "$count"}}}'

    example = [
        {
            "$group": {
                "_id": "$name",
                "type": "type",
                "total": {
                    "$sum": {
                        "$multiply": [
                            "$price",
                            "$count"
                        ]
                    }
                }
            }
        }
    ]
    assert to_aggregate(example) == '$aggregate: {$group: {_id: "$name", type: #0, total: {$sum: {$multiply: '\
                                    '["$price", "$count"]}}}}'
