﻿using Microsoft.EntityFrameworkCore.TestUtilities;

namespace Duckstax.EntityFramework.Ottergon.FunctionalTests.TestUtilities;

public class SampleProviderNorthwindTestStoreFactory : SampleProviderTestStoreFactory
{
    public static new SampleProviderNorthwindTestStoreFactory Instance { get; } = new();

    protected SampleProviderNorthwindTestStoreFactory()
    {
    }

    public override TestStore GetOrCreate(string storeName)
        => SampleProviderTestStore.GetOrCreate("northwind");

}