// This header intentionally has no include guards.

class preferences_node {
public:
  preferences_node(void);
  virtual ~preferences_node(void) {}

  bool handle_name_and_appendix(const boost::property_tree::ptree::value_type& it);

  const std::string& get_name(void) const { return name_; }
  int get_name_line_count(void) const { return name_line_count_; }
  const std::string& get_identifier(void) const { return identifier_; }

protected:
  std::string name_;
  int name_line_count_;

  std::string identifier_;
};

class preferences_checkbox_node : public preferences_node {
public:
  preferences_checkbox_node(void);
  preferences_checkbox_node(const preferences_checkbox_node& parent_node);

  void handle_item_child(const boost::property_tree::ptree::value_type& it);

  const std::string& get_name_for_filter(void) const { return name_for_filter_; }

private:
  std::string name_for_filter_;
};

class preferences_number_node : public preferences_node {
public:
  preferences_number_node(void);
  preferences_number_node(const preferences_number_node& parent_node);

  void handle_item_child(const boost::property_tree::ptree::value_type& it);

  int get_default_value(void) const { return default_value_; }
  int get_step(void) const { return step_; }
  const std::string& get_base_unit(void) const { return base_unit_; }

private:
  int default_value_;
  int step_;
  std::string base_unit_;
};

template <class T>
class preferences_node_tree {
public:
  typedef std::tr1::shared_ptr<preferences_node_tree> preferences_node_tree_ptr;
  typedef std::vector<preferences_node_tree_ptr> preferences_node_tree_ptrs;
  typedef std::tr1::shared_ptr<preferences_node_tree_ptrs> preferences_node_tree_ptrs_ptr;

  preferences_node_tree(void) {}
  preferences_node_tree(const T& parent_node) : node_(parent_node) {}

  void clear(void);
  void traverse_item(const boost::property_tree::ptree& pt, const xml_compiler& xml_compiler);
  const T& get_node(void) const { return node_; }
  const preferences_node_tree_ptrs_ptr& get_children(void) const { return children_; }

private:
  T node_;

  // We use shared_ptr<vector> to store children node by these reason.
  // * sizeof(shared_ptr) < sizeof(vector).
  // * children_ is mostly empty.
  preferences_node_tree_ptrs_ptr children_;
};

class preferences_node_loader {
public:
  preferences_node_loader(const xml_compiler& xml_compiler,
                          preferences_node_tree<preferences_checkbox_node>& preferences_checkbox_node_tree,
                          preferences_node_tree<preferences_number_node>& preferences_number_node_tree) :
    xml_compiler_(xml_compiler),
    preferences_checkbox_node_tree_(preferences_checkbox_node_tree),
    preferences_number_node_tree_(preferences_number_node_tree)
  {}

  void reload(void) const;

private:
  const xml_compiler& xml_compiler_;
  preferences_node_tree<preferences_checkbox_node>& preferences_checkbox_node_tree_;
  preferences_node_tree<preferences_number_node>& preferences_number_node_tree_;
};