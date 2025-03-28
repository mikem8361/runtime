// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System.Collections.Generic;
using System.Reflection;
using Xunit;

namespace System.ComponentModel.DataAnnotations.Tests
{
    public class CompareAttributeTests : ValidationAttributeTestBase
    {
        protected override IEnumerable<TestCase> ValidValues() => new TestCase[]
        {
            new TestCase(new CompareAttribute("CompareProperty"), "test", new ValidationContext(new CompareObject("test"))),
            new TestCase(new DerivedCompareAttribute("CompareProperty"), "a", new ValidationContext(new CompareObject("b")))
        };

        private static ValidationContext s_context = new ValidationContext(new CompareObject("a")) { DisplayName = "CurrentProperty" };
        protected override IEnumerable<TestCase> InvalidValues() => new TestCase[]
        {
            new TestCase(new CompareAttribute(nameof(CompareObject.CompareProperty)), "b", s_context),
            new TestCase(new CompareAttribute(nameof(CompareObject.ComparePropertyWithDisplayName)), "b", s_context),
            new TestCase(new CompareAttribute("NoSuchProperty"), "b", s_context),
            new TestCase(new CompareAttribute(nameof(CompareObject.CompareProperty)), "b", new ValidationContext(new CompareObjectSubClass("a")))
        };

        [Fact]
        public static void Constructor_NullOtherProperty_ThrowsArgumentNullException()
        {
            AssertExtensions.Throws<ArgumentNullException>("otherProperty", () => new CompareAttribute(null));
        }

        [Theory]
        [InlineData("OtherProperty")]
        [InlineData("")]
        public static void Constructor(string otherProperty)
        {
            CompareAttribute attribute = new CompareAttribute(otherProperty);
            Assert.Equal(otherProperty, attribute.OtherProperty);

            Assert.True(attribute.RequiresValidationContext);
        }

        [Fact]
        public static void Validate_Indexer_ThrowsArgumentException()
        {
            CompareAttribute attribute = new CompareAttribute("Item");
            AssertExtensions.Throws<ArgumentException>(null, () => attribute.Validate("b", s_context));
        }

        [Fact]
        public static void Validate_SetOnlyProperty_ThrowsArgumentException()
        {
            CompareAttribute attribute = new CompareAttribute(nameof(CompareObject.SetOnlyProperty));
            AssertExtensions.Throws<ArgumentException>(null, () => attribute.Validate("b", s_context));
        }

        [Fact]
        public static void Validate_LowerAndUpperPropertyName_Success()
        {
            CompareAttribute attribute = new CompareAttribute(nameof(CompareObject.comparepropertycased));
            Assert.NotNull(attribute.GetValidationResult("b", s_context).ErrorMessage);
            Assert.Equal(ValidationResult.Success, attribute.GetValidationResult(null, s_context));
            Assert.Equal(nameof(CompareObject.comparepropertycased), attribute.OtherPropertyDisplayName);
        }

        [Fact]
        public static void Validate_IncludesMemberName()
        {
            ValidationContext validationContext = new ValidationContext(new CompareObject("a"));
            validationContext.MemberName = nameof(CompareObject.CompareProperty);
            CompareAttribute attribute = new CompareAttribute(nameof(CompareObject.ComparePropertyCased));

            ValidationResult validationResult = attribute.GetValidationResult("b", validationContext);

            Assert.NotNull(validationResult.ErrorMessage);
            Assert.Equal(new[] { nameof(CompareObject.CompareProperty) }, validationResult.MemberNames);
        }

        [Fact]
        public static void Validate_PrivateProperty_ThrowsArgumentException()
        {
            CompareAttribute attribute = new CompareAttribute("PrivateProperty");
            Assert.Throws<ValidationException>(() => attribute.Validate("b", s_context));
        }

        [Fact]
        public static void Validate_PropertyHasDisplayName_UpdatesFormatErrorMessageToContainDisplayName()
        {
            CompareAttribute attribute = new CompareAttribute(nameof(CompareObject.ComparePropertyWithDisplayName));

            string oldErrorMessage = attribute.FormatErrorMessage("name");
            Assert.DoesNotContain("CustomDisplayName", oldErrorMessage);

            Assert.Throws<ValidationException>(() => attribute.Validate("test1", new ValidationContext(new CompareObject("test"))));

            string newErrorMessage = attribute.FormatErrorMessage("name");
            Assert.NotEqual(oldErrorMessage, newErrorMessage);
            Assert.Contains("CustomDisplayName", newErrorMessage);
        }

        [Fact]
        public static void IsValid_ValidationContextNull_ThrowsArgumentNullException()
        {
            var attribute = new CompareAttribute(nameof(CompareObject.ComparePropertyWithDisplayName));
            var compareObject = new CompareObject("test");

            Assert.Throws<ArgumentNullException>(() => attribute.IsValid(compareObject.CompareProperty));
        }

        private class DerivedCompareAttribute : CompareAttribute
        {
            public DerivedCompareAttribute(string otherProperty) : base(otherProperty) { }

            protected override ValidationResult IsValid(object value, ValidationContext context) => ValidationResult.Success;
        }

        private class CompareObject
        {
            public string CompareProperty { get; set; }

            [Display(Name = "CustomDisplayName")]
            public string ComparePropertyWithDisplayName { get; set; }

            public string this[int index] { get { return "abc"; } set { } }
            public string SetOnlyProperty { set { } }
            private string PrivateProperty { get; set; }

            public string ComparePropertyCased { get; set; }
            public string comparepropertycased { get; set; }

            public CompareObject(string otherValue)
            {
                CompareProperty = otherValue;
                ComparePropertyWithDisplayName = otherValue;
            }
        }

        private class CompareObjectSubClass : CompareObject
        {
            public CompareObjectSubClass(string otherValue) : base(otherValue) { }
        }
    }
}
