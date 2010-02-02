// Copyright (C) 2010  Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

// $Id$

#ifndef _ISC_DATA_H
#define _ISC_DATA_H 1

#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>
#include <stdexcept>
#include <dns/exceptions.h>

namespace isc { namespace data {

class Element;
// todo: describe the rationale behind ElementPtr?
typedef boost::shared_ptr<Element> ElementPtr;

///
/// \brief A standard Data module exception that is thrown if a function
/// is called for an Element that has a wrong type (e.g. int_value on a
/// ListElement)
///
// todo: include types and called function in the exception
class TypeError : public isc::dns::Exception {
public:
    TypeError(const char* file, size_t line, const char* what) :
        isc::dns::Exception(file, line, what) {}
};

///
/// \brief A standard Data module exception that is thrown if a parse
/// error is encountered when constructing an Element from a string
///
// i'd like to use Exception here but we need one that is derived from
// runtime_error (as this one is directly based on external data, and
// i want to add some values to any static data string that is provided)
class ParseError : public std::runtime_error {
public:
    ParseError(const std::string &err) : std::runtime_error(err) {};
};

///
/// \brief A standard Data module exception that is thrown if an error
/// is found when decoding an Element from wire format
///
class DecodeError : public std::exception {
public:
    DecodeError(std::string m = "Wire-format data is invalid") : msg(m) {}
    ~DecodeError() throw() {}
    const char* what() const throw() { return msg.c_str(); }
private:
    std::string msg;
};

///
/// \brief The \c Element class represents a piece of data, used by
/// the command channel and configuration parts.
///
/// An \c Element can contain simple types (int, real, string, bool and
/// None), and composite types (list and string->element maps)
///
/// Elements should in calling functions usually be referenced through
/// an \c ElementPtr, which can be created using the factory functions
/// \c Element::create() and \c Element::createFromString()
///
/// Notes to developers: Element is a base class, implemented by a
/// specific subclass for each type (IntElement, BoolElement, etc).
/// Element does define all functions for all types, and defaults to
/// raising a \c TypeError for functions that are not supported for
/// the type in question.
///
class Element {
    
private:
    // technically the type could be omitted; is it useful?
    // should we remove it or replace it with a pure virtual
    // function getType?
    int type;

protected:
    Element(int t) { type = t; }

public:
    enum types { integer, real, boolean, string, list, map };
    // base class; make dtor virtual
    virtual ~Element() {};

    /// \return the type of this element
    int getType() { return type; };
    
    // pure virtuals, every derived class must implement these

    /// Returns a string representing the Element and all its
    /// child elements; note that this is different from stringValue(),
    /// which only returns the single value of a StringElement
    /// A MapElement will be represented as { "name1": \<value1\>, "name2", \<value2\>, etc }
    /// A ListElement will be represented as [ \<item1\>, \<item2\>, etc ]
    /// All other elements will be represented directly
    ///
    /// \return std::string containing the string representation
    virtual std::string str() = 0;

    /// Returns the wireformat for the Element and all its child
    /// elements.
    ///
    /// \param omit_length If this is non-zero, the item length will
    ///        be omitted from the wire format
    /// \return std::string containing the element in wire format
    std::string toWire(int omit_length = 1);
    virtual void toWire(std::stringstream& out, int omit_length = 1) = 0;

    /// \name Type-specific getters
    ///
    ///
    /// \brief These functions only
    /// work on their corresponding Element type. For all other
    /// types, a TypeError is thrown.
    /// If you want an exception-safe getter method, use
    /// getValue() below
    //@{
    virtual int intValue() { dns_throw(TypeError, "intValue() called on non-integer Element"); };
    virtual double doubleValue() { dns_throw(TypeError, "doubleValue() called on non-double Element"); };
    virtual bool boolValue() { dns_throw(TypeError, "boolValue() called on non-Bool Element"); };
    virtual std::string stringValue() { dns_throw(TypeError, "stringValue() called on non-string Element"); };
    virtual const std::vector<boost::shared_ptr<Element> >& listValue() { dns_throw(TypeError, "listValue() called on non-list Element"); }; // replace with real exception or empty vector?
    virtual const std::map<std::string, boost::shared_ptr<Element> >& mapValue() { dns_throw(TypeError, "mapValue() called on non-map Element"); }; // replace with real exception or empty map?
    //@}

    /// \name Exception-safe getters
    ///
    /// \brief The getValue() functions return false if the given reference
    /// is of another type than the element contains
    /// By default it always returns false; the derived classes
    /// override the function for their type, copying their
    /// data to the given reference and returning true
    ///
    //@{
    virtual bool getValue(int& t) { return false; };
    virtual bool getValue(double& t) { return false; };
    virtual bool getValue(bool& t) { return false; };
    virtual bool getValue(std::string& t) { return false; };
    virtual bool getValue(std::vector<ElementPtr>& t) { return false; };
    virtual bool getValue(std::map<std::string, ElementPtr>& t) { return false; };
    //@}
    ///
    /// \name Exception-safe setters.
    ///
    /// \brief Return false if the Element is not
    /// the right type. Set the value and return true if the Elements
    /// is of the correct type
    ///
    //@{
    virtual bool setValue(const int v) { return false; };
    virtual bool setValue(const double v) { return false; };
    virtual bool setValue(const bool t) { return false; };
    virtual bool setValue(const std::string& v) { return false; };
    virtual bool setValue(const std::vector<ElementPtr>& v) { return false; };
    virtual bool setValue(const std::map<std::string, ElementPtr>& v) { return false; };
    //@}



    // Other functions for specific subtypes

    /// \name ListElement functions
    ///
    /// \brief If the Element on which these functions are called are not
    /// an instance of ListElement, a TypeError exception is thrown.
    //@{
    /// Returns the ElementPtr at the given index. If the index is out
    /// of bounds, this function throws an std::out_of_range exception.
    /// \param i The position of the ElementPtr to return
    virtual ElementPtr get(const int i) { dns_throw(TypeError, "get(int) called on a non-list Element"); };
    /// Sets the ElementPtr at the given index. If the index is out
    /// of bounds, this function throws an std::out_of_range exception.
    /// \param i The position of the ElementPtr to set
    /// \param element The ElementPtr to set at the position
    virtual void set(const size_t i, ElementPtr element) { dns_throw(TypeError, "set(int, element) called on a non-list Element"); };
    /// Adds an ElementPtr to the list
    /// \param element The ElementPtr to add
    virtual void add(ElementPtr element) { dns_throw(TypeError, "add() called on a non-list Element"); };
    /// Removes the element at the given position. If the index is out
    /// of nothing happens.
    /// \param i The index of the element to remove.
    virtual void remove(const int i) { dns_throw(TypeError, "remove(int) called on a non-list Element"); };
    /// Returns the number of elements in the list.
    virtual size_t size() { dns_throw(TypeError, "size() called on a non-list Element"); };
    //@}
    
    /// \name MapElement functions
    ///
    /// \brief If the Element on which these functions are called are not
    /// an instance of MapElement, a TypeError exception is thrown.
    //@{
    /// Returns the ElementPtr at the given key
    /// \param name The key of the Element to return
    /// \return The ElementPtr at the given key
    virtual ElementPtr get(const std::string& name) { dns_throw(TypeError, "get(string) called on a non-map Element"); } ;
    /// Sets the ElementPtr at the given key
    /// \param name The key of the Element to set
    virtual void set(const std::string& name, ElementPtr element) { dns_throw(TypeError, "set(name, element) called on a non-map Element"); };
    /// Remove the ElementPtr at the given key
    /// \param name The key of the Element to remove
    virtual void remove(const std::string& name) { dns_throw(TypeError, "remove(string) called on a non-map Element"); };
    /// Checks if there is data at the given key
    /// \param name The key of the Element to remove
    /// \return true if there is data at the key, false if not.
    virtual bool contains(const std::string& name) { dns_throw(TypeError, "contains(string) called on a non-map Element"); }
    /// Recursively finds any data at the given identifier. The
    /// identifier is a /-separated list of names of nested maps, with
    /// the last name being the leaf that is returned.
    ///
    /// For instance, if you have a MapElement that contains another
    /// MapElement at the key "foo", and that second MapElement contains
    /// Another Element at key "bar", the identifier for that last
    /// element from the first is "foo/bar".
    ///
    /// \param identifier The identifier of the element to find
    /// \return The ElementPtr at the given identifier. Returns a
    /// null ElementPtr if it is not found, which can be checked with
    /// Element::is_null(ElementPtr e).
    virtual ElementPtr find(const std::string& identifier) { dns_throw(TypeError, "find(string) called on a non-map Element"); };
    /// See \c Element::find()
    /// \param identifier The identifier of the element to find
    /// \param t Reference to store the resulting ElementPtr, if found.
    /// \return true if the element was found, false if not.
    virtual bool find(const std::string& identifier, ElementPtr& t) { return false; };
    //@}

    /// \name Factory functions
    
    // TODO: should we move all factory functions to a different class
    // so as not to burden the Element base with too many functions?
    // and/or perhaps even to a separate header?

    /// \name Direct factory functions
    /// \brief These functions simply wrap the given data directly
    /// in an Element object, and return a reference to it, in the form
    /// of an \c ElementPtr.
    /// If there is a memory allocation problem, these functions will
    /// return a NULL ElementPtr, which can be checked with
    /// Element::is_null(ElementPtr ep).
    //@{
    static ElementPtr create(const int i);
    static ElementPtr create(const double d);
    static ElementPtr create(const bool b);
    static ElementPtr create(const std::string& s);
    // need both std:string and char *, since c++ will match
    // bool before std::string when you pass it a char *
    static ElementPtr create(const char *s) { return create(std::string(s)); }; 
    static ElementPtr create(const std::vector<ElementPtr>& v);
    static ElementPtr create(const std::map<std::string, ElementPtr>& m);
    //@}

    /// \name Compound factory functions

    /// \brief These functions will parse the given string representation
    /// of a compound element. If there is a parse error, an exception
    /// of the type isc::data::ParseError is thrown.

    //@{
    /// Creates an Element from the given string
    /// \param in The string to parse the element from
    /// \return An ElementPtr that contains the element(s) specified
    /// in the given string.
    static ElementPtr createFromString(const std::string& in);
    /// Creates an Element from the given input stream
    /// \param in The string to parse the element from
    /// \return An ElementPtr that contains the element(s) specified
    /// in the given input stream.
    static ElementPtr createFromString(std::istream& in) throw(ParseError);
    /// Creates an Element from the given input stream, where we keep
    /// track of the location in the stream for error reporting.
    ///
    /// \param in The string to parse the element from
    /// \param line A reference to the int where the function keeps
    /// track of the current line.
    /// \param line A reference to the int where the function keeps
    /// track of the current position within the current line.
    /// \return An ElementPtr that contains the element(s) specified
    /// in the given input stream.
    // make this one private?
    static ElementPtr createFromString(std::istream& in, const std::string& file, int& line, int &pos) throw(ParseError);
    //@}

    /// \name Wire format factory functions

    /// These function pparse the wireformat at the given stringstream
    /// (of the given length). If there is a parse error an exception
    /// of the type isc::cc::DecodeError is raised.
    
    //@{
    /// Creates an Element from the wire format in the given
    /// stringstream of the given length.
    /// \param in The input stringstream.
    /// \param length The length of the wireformat data in the stream
    /// \return ElementPtr with the data that is parsed.
    static ElementPtr fromWire(std::stringstream& in, int length);
    /// Creates an Element from the wire format in the given string
    /// \param s The input string
    /// \return ElementPtr with the data that is parsed.
    static ElementPtr fromWire(const std::string& s);
    //@}
};

class IntElement : public Element {
    int i;

public:
    IntElement(int v) : Element(integer), i(v) { };
    int intValue() { return i; }
    bool getValue(int& t) { t = i; return true; };
    bool setValue(const int v) { i = v; return true; };
    std::string str();
    void toWire(std::stringstream& ss, int omit_length = 1);
};

class DoubleElement : public Element {
    double d;

public:
    DoubleElement(double v) : Element(real), d(v) {};
    double doubleValue() { return d; }
    bool getValue(double& t) { t = d; return true; };
    bool setValue(const double v) { d = v; return true; };
    std::string str();
    void toWire(std::stringstream& ss, int omit_length = 1);
};

class BoolElement : public Element {
    bool b;

public:
    BoolElement(const bool v) : Element(boolean), b(v) {};
    bool boolValue() { return b; }
    bool getValue(bool& t) { t = b; return true; };
    bool setValue(const bool v) { b = v; return true; };
    std::string str();
    void toWire(std::stringstream& ss, int omit_length = 1);
};

class StringElement : public Element {
    std::string s;

public:
    StringElement(std::string v) : Element(string), s(v) {};
    std::string stringValue() { return s; };
    bool getValue(std::string& t) { t = s; return true; };
    bool setValue(const std::string& v) { s = v; return true; };
    std::string str();
    void toWire(std::stringstream& ss, int omit_length = 1);
};

class ListElement : public Element {
    std::vector<ElementPtr> l;

public:
    ListElement(std::vector<ElementPtr> v) : Element(list), l(v) {};
    const std::vector<ElementPtr>& listValue() { return l; }
    bool getValue(std::vector<ElementPtr>& t) { t = l; return true; };
    bool setValue(const std::vector<ElementPtr>& v) { l = v; return true; };
    ElementPtr get(int i) { return l.at(i); };
    void set(size_t i, ElementPtr e) { if (i <= l.size()) {l[i] = e;} else { throw std::out_of_range("vector::_M_range_check"); } };
    void add(ElementPtr e) { l.push_back(e); };
    void remove(int i) { l.erase(l.begin() + i); };
    std::string str();
    void toWire(std::stringstream& ss, int omit_length = 1);
    size_t size() { return l.size(); }
};

class MapElement : public Element {
    std::map<std::string, ElementPtr> m;

public:
    MapElement(std::map<std::string, ElementPtr> v) : Element(map), m(v) {};
    const std::map<std::string, ElementPtr>& mapValue() { return m; }
    bool getValue(std::map<std::string, ElementPtr>& t) { t = m; return true; };
    bool setValue(std::map<std::string, ElementPtr>& v) { m = v; return true; };
    ElementPtr get(const std::string& s) { return m[s]; };
    void set(const std::string& s, ElementPtr p) { m[s] = p; };
    void remove(const std::string& s) { m.erase(s); }
    bool contains(const std::string& s) { return m.find(s) != m.end(); }
    std::string str();
    void toWire(std::stringstream& ss, int omit_length = 1);
    
    //
    // Encode into the CC wire format.
    //
    void toWire(std::ostream& ss);

    // we should name the two finds better...
    // find the element at id; raises TypeError if one of the
    // elements at path except the one we're looking for is not a
    // mapelement.
    // returns an empty element if the item could not be found
    ElementPtr find(const std::string& id);

    // find the Element at 'id', and store the element pointer in t
    // returns true if found, or false if not found (either because
    // it doesnt exist or one of the elements in the path is not
    // a MapElement)
    bool find(const std::string& id, ElementPtr& t);
};

/// Checks whether the given ElementPtr is a NULL pointer
/// \param p The ElementPtr to check
/// \return true if it is NULL, false if not.
bool isNull(ElementPtr p);


} }

///
/// \brief Insert the Element as a string into stream.
///
/// This method converts the \c ElemetPtr into a string with
/// \c Element::str() and inserts it into the
/// output stream \c out.
///
/// This function overloads the global operator<< to behave as described in
/// ostream::operator<< but applied to \c ElementPtr objects.
///
/// \param os A \c std::ostream object on which the insertion operation is
/// performed.
/// \param e The \c ElementPtr object to insert.
/// \return A reference to the same \c std::ostream object referenced by
/// parameter \c os after the insertion operation.
std::ostream& operator <<(std::ostream &out, const isc::data::ElementPtr& e);

#endif // _ISC_DATA_H
